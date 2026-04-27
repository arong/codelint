import asyncio
import time
import os
import random
import uuid
from typing import Dict, Optional, Union
import tiktoken
import httpx
from openai import AsyncOpenAI


class GLMReviewClient:
    def __init__(self, base_url: str, model: str, api_key: Optional[str] = None):
        """
        Initialize the GLM Review Client.
        
        Args:
            base_url: Base URL for the GLM API
            model: Model name to use (e.g., glm-4)
            api_key: API key for authentication (defaults to OPENAI_API_KEY env var)
        """
        self.base_url = base_url
        self.model = model
        self.api_key = api_key or os.getenv("OPENAI_API_KEY")
        
        if not self.api_key:
            raise ValueError("API key must be provided either as parameter or through OPENAI_API_KEY environment variable")
        
        self.client = AsyncOpenAI(
            base_url=self.base_url,
            api_key=self.api_key,
        )
        
        # Use cl100k_base encoding as default, though this might vary by model
        try:
            self.tokenizer = tiktoken.get_encoding("cl100k_base")
        except KeyError:
            # Fallback to cl100k_base which works well for most models
            self.tokenizer = tiktoken.encoding_for_model("gpt-4")
    
    def count_tokens(self, text: str) -> int:
        """
        Count the number of tokens in the given text.
        
        Args:
            text: Text to count tokens for
            
        Returns:
            Number of tokens in the text
        """
        return len(self.tokenizer.encode(text))
    
    async def _make_request_with_retry(self, func, *args, **kwargs):
        """
        Make an API request with exponential backoff retry logic.
        
        Args:
            func: The async function to call
            *args, **kwargs: Arguments to pass to the function
            
        Returns:
            Result of the function call
            
        Raises:
            Exception: After all retry attempts are exhausted
        """
        max_retries = 3
        timeout = 120  # 120 seconds per request
        
        for attempt in range(max_retries + 1):
            try:
                # Create a timeout context for the request
                timeout_obj = httpx.Timeout(timeout=timeout)
                
                # Make the request
                result = await func(*args, **kwargs)
                return result
                
            except httpx.HTTPStatusError as e:
                status_code = e.response.status_code
                
                # Handle specific HTTP errors
                if status_code == 429:  # Rate limit
                    print(f"Rate limit exceeded (429). Attempt {attempt + 1}/{max_retries + 1}")
                elif status_code == 401:  # Unauthorized
                    print(f"Unauthorized (401). Invalid API key. Attempt {attempt + 1}/{max_retries + 1}")
                    if attempt == max_retries:  # Final attempt failed
                        raise  # Don't retry 401 errors as they indicate invalid credentials
                elif 500 <= status_code < 600:  # Server errors
                    print(f"Server error ({status_code}). Attempt {attempt + 1}/{max_retries + 1}")
                else:
                    print(f"HTTP error ({status_code}). Attempt {attempt + 1}/{max_retries + 1}")
                
                if attempt == max_retries:
                    raise  # Re-raise if we're out of retries
                    
                # Calculate backoff delay: 1s -> 2s -> 4s (doubling each time)
                backoff_delay = min(4, 2 ** attempt)  # Cap at 4 seconds
                await asyncio.sleep(backoff_delay)
                
            except asyncio.TimeoutError:
                print(f"Request timed out after {timeout}s. Attempt {attempt + 1}/{max_retries + 1}")
                if attempt == max_retries:
                    raise
                backoff_delay = min(4, 2 ** attempt)
                await asyncio.sleep(backoff_delay)
                
            except Exception as e:
                print(f"Unexpected error during request: {e}. Attempt {attempt + 1}/{max_retries + 1}")
                if attempt == max_retries:
                    raise
                backoff_delay = min(4, 2 ** attempt)
                await asyncio.sleep(backoff_delay)
    
    def validate_key_sync(self) -> bool:
        """
        Synchronous version to validate the API key by making a simple request.
        
        Returns:
            True if the API key is valid, False otherwise
        """
        import requests
        
        headers = {
            "Authorization": f"Bearer {self.api_key}",
            "Content-Type": "application/json"
        }
        
        # Make a simple request to test if the key is valid
        try:
            # Using a minimal chat completion request to test the API key
            response = requests.post(
                f"{self.base_url}/chat/completions",
                headers=headers,
                json={
                    "model": self.model,
                    "messages": [{"role": "user", "content": "Hi"}],
                    "max_tokens": 5
                },
                timeout=30
            )
            
            return response.status_code in [200, 400]  # 400 means key is valid but request is invalid
        except requests.RequestException:
            return False
    
    async def validate_key(self) -> bool:
        """
        Validate the API key by making a simple request.
        
        Returns:
            True if the API key is valid, False otherwise
        """
        try:
            result = await self._make_request_with_retry(
                self.client.chat.completions.create,
                model=self.model,
                messages=[{"role": "user", "content": "Hi"}],
                max_tokens=5
            )
            return True  # If we get here without exception, key should be valid
        except Exception as e:
            print(f"API key validation failed: {e}")
            return False
    
    async def review(self, content: str) -> Dict:
        """
        Send content to the GLM model for review and return the response.
        
        Args:
            content: Content to review
            
        Returns:
            Response from the GLM API as a dictionary
        """
        try:
            result = await self._make_request_with_retry(
                self.client.chat.completions.create,
                model=self.model,
                messages=[
                    {
                        "role": "user", 
                        "content": content
                    }
                ],
                timeout=120
            )
            
            # Convert the response to a dict
            response_dict = {
                'id': result.id,
                'choices': [
                    {
                        'index': choice.index,
                        'message': {
                            'role': choice.message.role,
                            'content': choice.message.content
                        },
                        'finish_reason': choice.finish_reason
                    } for choice in result.choices
                ],
                'created': result.created,
                'model': result.model,
                'usage': {
                    'prompt_tokens': result.usage.prompt_tokens if hasattr(result.usage, 'prompt_tokens') else 0,
                    'completion_tokens': result.usage.completion_tokens if hasattr(result.usage, 'completion_tokens') else 0,
                    'total_tokens': result.usage.total_tokens if hasattr(result.usage, 'total_tokens') else 0
                }
            }
            
            return response_dict
            
        except httpx.HTTPStatusError as e:
            status_code = e.response.status_code
            error_detail = {
                'error': True,
                'status_code': status_code,
                'message': str(e),
                'response_text': e.response.text
            }
            return error_detail
        except Exception as e:
            error_detail = {
                'error': True,
                'message': str(e)
            }
            return error_detail


class MockGLMReviewClient:
    """Mock API client that generates simulated responses with estimated tokens for testing."""
    
    MOCK_FINDINGS_TEMPLATES = [
        {
            "severity": "CRITICAL",
            "category": "memory_safety",
            "description": "Raw pointer ownership without smart pointer management",
            "original": "int* data = new int[size];",
            "suggestion": "auto data = std::make_unique<int[]>(size);",
            "explanation": "Raw pointers require manual memory management and are prone to leaks"
        },
        {
            "severity": "HIGH",
            "category": "undefined_behavior",
            "description": "Signed integer overflow potential in arithmetic operation",
            "original": "int result = a * b;",
            "suggestion": "int64_t result = static_cast<int64_t>(a) * b;",
            "explanation": "Multiplication can overflow for large values causing undefined behavior"
        },
        {
            "severity": "HIGH",
            "category": "error_handling",
            "description": "Unchecked return value from system call",
            "original": "read(fd, buffer, size);",
            "suggestion": "ssize_t n = read(fd, buffer, size); if (n < 0) handle_error();",
            "explanation": "System calls can fail; ignoring return values hides errors"
        },
        {
            "severity": "MEDIUM",
            "category": "code_quality",
            "description": "Missing const correctness on member function",
            "original": "int getValue() { return value_; }",
            "suggestion": "int getValue() const { return value_; }",
            "explanation": "Const correctness enables compiler optimizations and safer interfaces"
        },
        {
            "severity": "MEDIUM",
            "category": "thread_safety",
            "description": "Shared mutable state without synchronization",
            "original": "counter++;",
            "suggestion": "std::lock_guard<std::mutex> lock(mutex_); counter++;",
            "explanation": "Concurrent modification without locks causes data races"
        },
        {
            "severity": "LOW",
            "category": "code_quality",
            "description": "Unnecessary copy of large object in parameter",
            "original": "void process(std::string data) {",
            "suggestion": "void process(const std::string& data) {",
            "explanation": "Pass by value creates unnecessary copies, prefer const reference"
        },
        {
            "severity": "LOW",
            "category": "code_quality",
            "description": "Using namespace std pollutes global namespace",
            "original": "using namespace std;",
            "suggestion": "using std::vector;",
            "explanation": "Broad 'using namespace' imports create naming conflicts and reduce clarity"
        },
        {
            "severity": "CRITICAL",
            "category": "memory_safety",
            "description": "Use-after-free potential when accessing deleted object",
            "original": "delete ptr; ptr->method();",
            "suggestion": "delete ptr; ptr = nullptr;",
            "explanation": "Accessing object after deletion causes undefined behavior"
        },
        {
            "severity": "HIGH",
            "category": "memory_safety",
            "description": "Buffer overflow risk with unchecked array access",
            "original": "arr[index] = value;",
            "suggestion": "if (index >= 0 && index < size) arr[index] = value;",
            "explanation": "Out-of-bounds access causes memory corruption and security vulnerabilities"
        },
        {
            "severity": "MEDIUM",
            "category": "architecture",
            "description": "High coupling between modules through global state",
            "original": "extern GlobalConfig g_config;",
            "suggestion": "class ConfigProvider { virtual Config get() = 0; };",
            "explanation": "Global state creates hidden dependencies and complicates testing"
        },
    ]
    
    SEVERITIES = ["CRITICAL", "HIGH", "MEDIUM", "LOW"]
    SEVERITY_WEIGHTS = [0.1, 0.25, 0.35, 0.30]
    
    def __init__(self):
        self.total_prompt_tokens = 0
        self.total_completion_tokens = 0
        self.request_count = 0
        try:
            self.tokenizer = tiktoken.get_encoding("cl100k_base")
        except KeyError:
            self.tokenizer = tiktoken.encoding_for_model("gpt-4")
    
    def count_tokens(self, text: str) -> int:
        return len(self.tokenizer.encode(text))
    
    def _generate_findings(self, prompt: str, file_paths: list) -> list:
        prompt_tokens = self.count_tokens(prompt)
        num_findings = random.randint(2, min(6, max(2, len(file_paths) * 2)))
        findings = []
        available_lines = {}
        for fp in file_paths:
            available_lines[fp] = random.sample(range(10, 250), min(num_findings, 4))
        for i in range(num_findings):
            template = random.choice(self.MOCK_FINDINGS_TEMPLATES)
            file_path = random.choice(file_paths)
            line = available_lines[file_path].pop(0) if available_lines[file_path] else random.randint(10, 200)
            severity = random.choices(self.SEVERITIES, weights=self.SEVERITY_WEIGHTS, k=1)[0]
            findings.append({
                "file": file_path,
                "line": line,
                "severity": severity,
                "category": template["category"],
                "description": template["description"],
                "original": template["original"],
                "suggestion": template["suggestion"],
                "explanation": template["explanation"],
            })
        completion_tokens = random.randint(800, 2000)
        self.total_prompt_tokens += prompt_tokens
        self.total_completion_tokens += completion_tokens
        self.request_count += 1
        return findings
    
    async def review(self, content: str) -> Dict:
        import re
        file_pattern = r"=== START OF FILE: (.+?) ==="
        file_paths = re.findall(file_pattern, content)
        findings = self._generate_findings(content, file_paths)
        yaml_output = "findings:\n"
        for f in findings:
            yaml_output += f"""  - file: "{f["file"]}"
    line: {f["line"]}
    severity: "{f["severity"]}"
    category: "{f["category"]}"
    description: "{f["description"]}"
    original: "{f["original"]}"
    suggestion: "{f["suggestion"]}"
    explanation: "{f["explanation"]}"
"""
        return {
            "id": str(uuid.uuid4()),
            "choices": [{
                "index": 0,
                "message": {
                    "role": "assistant",
                    "content": yaml_output,
                },
                "finish_reason": "stop",
            }],
            "created": int(time.time()),
            "model": "mock-simulation",
            "usage": {
                "prompt_tokens": self.count_tokens(content),
                "completion_tokens": self.count_tokens(yaml_output),
                "total_tokens": self.count_tokens(content) + self.count_tokens(yaml_output),
            },
        }
    
    async def validate_key(self) -> bool:
        return True