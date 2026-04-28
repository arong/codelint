#!/usr/bin/env python3
"""
Review.py - File discovery and chunking tool for C++ code review
"""

import os
import sys
import argparse
import logging
import time
import yaml
from pathlib import Path
from typing import List, Dict, Any
import pathspec
import asyncio

from api import GLMReviewClient, MockGLMReviewClient

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)


def discover_files(target: str) -> List[str]:
    """
    Walks directory and finds C++ files respecting .gitignore patterns
    Extensions supported: .cpp, .cc, .cxx, .C, .h, .hpp, .hxx, .H
    Skips binary files and files with >5000 lines
    """
    supported_extensions = {'.cpp', '.cc', '.cxx', '.C', '.h', '.hpp', '.hxx', '.H'}

    # Load gitignore patterns if .gitignore exists
    gitignore_path = os.path.join(target, '.gitignore') if os.path.isfile(target) else os.path.join(os.path.dirname(target), '.gitignore')
    if os.path.exists(gitignore_path):
        with open(gitignore_path, 'r', encoding='utf-8') as f:
            gitignore_lines = f.readlines()
        spec = pathspec.PathSpec.from_lines(pathspec.patterns.GitWildMatchPattern, gitignore_lines)
    else:
        spec = pathspec.PathSpec.from_lines(pathspec.patterns.GitWildMatchPattern, [])

    cpp_files = []

    if os.path.isfile(target):
        target_dir = os.path.dirname(target)
        files_to_check = [target]
    else:
        target_dir = target
        files_to_check = []
        for root, dirs, files in os.walk(target_dir):
            for file in files:
                file_path = os.path.join(root, file)
                # Check if file is in .gitignore
                rel_file_path = os.path.relpath(file_path, target_dir)
                if spec.match_file(rel_file_path):
                    continue  # Skip gitignored files

                _, ext = os.path.splitext(file)
                if ext in supported_extensions:
                    files_to_check.append(file_path)

    for file_path in files_to_check:
        # Check for binary files
        try:
            with open(file_path, 'rb') as f:
                if b'\x00' in f.read():
                    logger.debug(f"Skipping binary file: {file_path}")
                    continue
        except IOError:
            logger.warning(f"Could not read file: {file_path}")
            continue

        # Check for line count
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                line_count = sum(1 for _ in f)
                if line_count > 5000:
                    logger.warning(f"Skipping file with {line_count} lines: {file_path}")
                    continue
        except UnicodeDecodeError:
            logger.debug(f"Skipping non-text file: {file_path}")
            continue
        except IOError:
            logger.warning(f"Could not read file: {file_path}")
            continue

        cpp_files.append(file_path)

    return cpp_files


def chunk_files(files: List[str], max_chunk_size: int = 3) -> List[List[str]]:
    """
    Groups files in chunks of max_chunk_size (default 3).
    Puts matching .cpp and .h files from same directory together.
    Prefers files from same directory in same chunk.
    """
    # Group files by directory
    dir_files: Dict[str, List[str]] = {}
    for file in files:
        directory = os.path.dirname(file)
        if directory not in dir_files:
            dir_files[directory] = []
        dir_files[directory].append(file)

    # Now create chunks while trying to keep same directory files together AND pair .cpp/.h files
    chunks = []
    current_chunk = []

    # First, process directories individually to pair .cpp/.h when possible
    for directory, dir_file_list in dir_files.items():
        # Separate source and header files
        src_files = []
        header_files = []

        for file in dir_file_list:
            ext = os.path.splitext(file)[1]
            if ext in ['.cpp', '.cc', '.cxx', '.C']:
                src_files.append(file)
            elif ext in ['.h', '.hpp', '.hxx', '.H']:
                header_files.append(file)

        # Form pairs while we can
        paired_files = []
        processed_headers = set()

        for src_file in src_files:
            src_base = os.path.splitext(os.path.basename(src_file))[0]
            # Look for corresponding header file
            matched_header = None
            for header_file in header_files:
                if os.path.splitext(os.path.basename(header_file))[0] == src_base and header_file not in processed_headers:
                    matched_header = header_file
                    break

            if matched_header:
                # Add in order: source then header
                paired_files.extend([src_file, matched_header])
                processed_headers.add(matched_header)
            else:
                # Add just the source file
                paired_files.append(src_file)

        # Add any remaining unmatched headers
        for header_file in header_files:
            if header_file not in processed_headers:
                paired_files.append(header_file)

        # Try to add all files from this directory to chunks respecting max size
        temp_idx = 0
        temp_files = paired_files[:]

        while temp_files:
            for i, file in enumerate(temp_files[:]):
                if len(current_chunk) < max_chunk_size:
                    current_chunk.append(file)
                    temp_files.remove(file)
                else:
                    break

            if len(current_chunk) >= max_chunk_size or not temp_files:
                if current_chunk:
                    chunks.append(current_chunk)
                    current_chunk = []

    # Handle any remaining files that couldn't be grouped by directory
    all_files_set = set(files)
    chunked_files_set = set()
    for chunk in chunks:
        chunked_files_set.update(chunk)

    remaining_files = list(all_files_set - chunked_files_set)

    for file in remaining_files:
        if len(current_chunk) < max_chunk_size:
            current_chunk.append(file)
        else:
            chunks.append(current_chunk)
            current_chunk = [file]

    # Add last chunk if it has files
    if current_chunk:
        chunks.append(current_chunk)

    return chunks


def read_chunk_content(chunk: List[str]) -> Dict[str, str]:
    """
    Reads the content of all files in a chunk.

    Args:
        chunk: List of file paths

    Returns:
        Dictionary mapping filename to content
    """
    content_map = {}
    for file_path in chunk:
        with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
            content_map[file_path] = f.read()
    return content_map


def build_review_prompt(chunk_content: Dict[str, str], system_prompt: str) -> str:
    """
    Builds the prompt for the review by combining system prompt with file contents.

    Args:
        chunk_content: Dictionary mapping file paths to their content
        system_prompt: The system prompt containing instructions

    Returns:
        Full prompt to send to the API
    """
    prompt = []
    prompt.append(system_prompt)
    prompt.append("\n\nHere are the files to review:\n\n")

    for file_path, content in chunk_content.items():
        prompt.append(f"=== START OF FILE: {file_path} ===\n")
        prompt.append(content)
        prompt.append(f"\n=== END OF FILE: {file_path} ===\n\n")

    return "".join(prompt)


def parse_api_response(response: str) -> List[Dict[str, Any]]:
    """
    Parses the YAML structure from the API response.

    Args:
        response: API response string containing YAML

    Returns:
        List of findings
    """
    try:
        # Extract YAML part if wrapped in triple backticks
        if "```yaml" in response:
            start_idx = response.find("```yaml") + len("```yaml")
            end_idx = response.find("```", start_idx)
            yaml_content = response[start_idx:end_idx].strip()
        elif "```" in response:
            start_idx = response.find("```") + len("```")
            end_idx = response.find("```", start_idx)
            yaml_content = response[start_idx:end_idx].strip()
        else:
            yaml_content = response.strip()

        data = yaml.safe_load(yaml_content)
        if isinstance(data, dict) and "findings" in data:
            return data["findings"]
        elif isinstance(data, list):
            return data
        else:
            logger.error(f"Invalid response format, expected dict with findings or list, got: {type(data)}")
            return []
    except yaml.YAMLError as e:
        logger.error(f"Failed to parse YAML response: {e}")
        logger.debug(f"Response content: {response}")
        return []
    except Exception as e:
        logger.error(f"Unexpected error parsing response: {e}")
        logger.debug(f"Response content: {response}")
        return []


async def review_chunk(chunk: List[str], client: GLMReviewClient, system_prompt: str) -> List[Dict[str, Any]]:
    """
    Reviews a chunk of files and returns parsed findings.

    Args:
        chunk: List of file paths to review
        client: GLM Review Client instance
        system_prompt: The system prompt containing instructions

    Returns:
        List of parsed findings
    """
    chunk_content = read_chunk_content(chunk)
    prompt = build_review_prompt(chunk_content, system_prompt)

    response = await client.review(prompt)

    if 'error' in response:
        logger.error(f"Error from API: {response.get('message', 'Unknown error')}")
        return []

    content = response['choices'][0]['message']['content'] if response.get('choices') else ""
    findings = parse_api_response(content)

    return findings


def write_markdown_report(findings: List[Dict[str, Any]], output_path: str, files_processed: List[str]) -> None:
    """
    Writes findings to a markdown report file.

    Args:
        findings: List of findings to report
        output_path: Path to output file
        files_processed: List of files that were processed/analyzed
    """
    report = []

    # Create summary
    report.append("# Code Review Report\n\n")
    report.append(f"## Summary\n\n")
    report.append(f"- Files analyzed: {len(set([f['file'] for f in findings])) if findings else 0}/{len(set(files_processed))}\n")

    # Count issues by severity
    severity_counts = {'CRITICAL': 0, 'HIGH': 0, 'MEDIUM': 0, 'LOW': 0}
    for finding in findings:
        severity = finding.get('severity', '').upper()
        if severity in severity_counts:
            severity_counts[severity] += 1

    report.append("- Issues found:\n")
    for severity, count in severity_counts.items():
        if count > 0:
            report.append(f"  - {severity.title()}: {count}\n")

    report.append('\n')

    # Create per-issue sections
    if findings:
        report.append('## Issues Found\n\n')

        for i, finding in enumerate(findings, 1):
            file_path = finding.get('file', 'unknown')
            line_num = finding.get('line', '?')
            severity = finding.get('severity', 'UNKNOWN').title()
            category = finding.get('category', 'General')
            description = finding.get('description', '')
            original = finding.get('original', '')
            suggestion = finding.get('suggestion', '')
            explanation = finding.get('explanation', '')

            report.append(f"### Issue {i}\n\n")
            report.append(f"- **File**: `{file_path}`\n")
            report.append(f"- **Line**: {line_num}\n")
            report.append(f"- **Severity**: {severity}\n")
            report.append(f"- **Category**: {category}\n")
            report.append(f"- **Description**: {description}\n\n")

            if original and suggestion:
                report.append("**Original Code:**\n")
                report.append("```cpp\n")
                report.append(original + "\n")
                report.append("```\n\n")

                report.append("**Suggested Fix:**\n")
                report.append("```cpp\n")
                report.append(suggestion + "\n")
                report.append("```\n\n")
            elif original:
                report.append("**Code:**\n")
                report.append("```cpp\n")
                report.append(original + "\n")
                report.append("```\n\n")

            if explanation:
                report.append(f"**Explanation**: {explanation}\n\n")
            else:
                report.append("\n")

    # Write report to file
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("".join(report))


async def run_review_pipeline(args: argparse.Namespace) -> float:
    """
    Runs the full review pipeline.

    Args:
        args: CLI arguments

    Returns:
        Success rate (proportion of chunks processed successfully)
    """
    pipeline_start = time.time()

    # Load system prompt
    try:
        with open("prompts/review-system.md", 'r', encoding='utf-8') as f:
            system_prompt = f.read()
    except FileNotFoundError:
        logger.error("System prompt file prompts/review-system.md not found")
        return 0.0
    except Exception as e:
        logger.error(f"Error reading system prompt: {e}")
        return 0.0

    # Discover files
    logger.info(f"Discovering files in: {args.target}")
    files = discover_files(args.target)
    if not files:
        logger.warning("No C++ files found to review")
        return 1.0  # Considered success if no files exist to review

    logger.info(f"Found {len(files)} files to review")

    # Chunk files
    chunks = chunk_files(files)
    logger.info(f"Created {len(chunks)} chunks to review")

    if args.dry_run:
        logger.info("DRY RUN MODE - Skipping actual review")
        for i, chunk in enumerate(chunks, 1):
            file_names = [os.path.basename(f) for f in chunk]
            logger.info(f"Chunk {i}/{len(chunks)}: {', '.join(file_names)}")
        return 1.0

    # Initialize API client
    if args.simulate:
        logger.info("SIMULATION MODE - Using mock API client (no charges)")
        client = MockGLMReviewClient()
    else:
        client = GLMReviewClient(
            base_url=args.base_url or os.environ.get("GLM_BASE_URL", "https://open.bigmodel.cn/api/paas/v4"),
            model=args.model or os.environ.get("GLM_MODEL", "glm-4"),
            api_key=args.api_key
        )

    # Validate the API key (skip in simulate mode)
    if not args.simulate:
        if not await client.validate_key():
            logger.error("API key validation failed. Please ensure your API key is valid.")
            return 0.0  # Failed validation implies failure
    else:
        logger.info("API key validation skipped (simulation mode)")

    # Run the review on all chunks
    all_findings = []
    successful_chunks = 0

    for i, chunk in enumerate(chunks, 1):
        file_names = [os.path.basename(f) for f in chunk]
        logger.info(f"Chunk {i}/{len(chunks)}: reviewing {', '.join(file_names)}...")

        try:
            findings = await review_chunk(chunk, client, system_prompt)
            all_findings.extend(findings)
            successful_chunks += 1
            logger.debug(f"Reviewed chunk {i}: {len(findings)} issues found")
        except Exception as e:
            logger.error(f"Error reviewing chunk {i}: {e}")
            # Log detailed error info for debugging
            logger.debug(f"Files in failed chunk: {chunk}")

    success_rate = successful_chunks / len(chunks) if len(chunks) > 0 else 1.0
    logger.info(f"Review completed: {successful_chunks}/{len(chunks)} chunks successful ({success_rate:.1%})")

    # Token and cost estimation
    total_prompt_tokens = 0
    total_completion_tokens = 0
    if hasattr(client, 'total_prompt_tokens'):
        total_prompt_tokens = client.total_prompt_tokens
        total_completion_tokens = client.total_completion_tokens
    else:
        for chunk_idx in range(len(chunks)):
            chunk_content = read_chunk_content(chunks[chunk_idx])
            prompt = build_review_prompt(chunk_content, system_prompt)
            total_prompt_tokens += client.count_tokens(prompt)

    total_tokens = total_prompt_tokens + total_completion_tokens
    prompt_cost = total_prompt_tokens / 1_000_000 * 0.005
    completion_cost = total_completion_tokens / 1_000_000 * 0.015
    estimated_cost = prompt_cost + completion_cost

    logger.info(f"Token usage: {total_prompt_tokens} prompt + {total_completion_tokens} completion = {total_tokens} total")
    logger.info(f"Estimated cost: ${estimated_cost:.4f} (prompt: ${prompt_cost:.4f}, completion: ${completion_cost:.4f})")
    elapsed = time.time() - pipeline_start
    logger.info(f"Pipeline completed in {elapsed:.2f} seconds")

    # Write the results to the output file
    output_file = args.output or "review-report.md"
    write_markdown_report(all_findings, output_file, files)
    logger.info(f"Report written to: {output_file}")

    return success_rate


def main():
    parser = argparse.ArgumentParser(description='Discover and chunk C++ files for GLM review')
    parser.add_argument('--target', required=True, help='Directory or file to process')
    parser.add_argument('--api-key', required=True, help='API key for GLM service')
    parser.add_argument('--output', default='review-report.md', help='Output markdown report file')
    parser.add_argument('--dry-run', action='store_true', help='Show what would be processed without actually processing')
    parser.add_argument('--base-url', help='GLM API base URL')
    parser.add_argument('--model', help='Model name for GLM API (default: glm-4)')
    parser.add_argument('--simulate', action='store_true',
                        help='Simulate API calls for validation without actual charges')

    args = parser.parse_args()

    # Run the async pipeline
    success_rate = asyncio.run(run_review_pipeline(args))

    # Exit with code based on success rate
    if success_rate >= 0.8:
        sys.exit(0)  # Success
    else:
        sys.exit(1)  # Failure


if __name__ == "__main__":
    main()
