1. find_global: 查找全局变量并输出。
2. find_singleton: 查找单例（meyer's singleton）
3. check_init: 查找变量初始化相关的问题，包含
   - 未显示初始化变量,比如`int c;`
   - 使用 `=`号初始化的变量需要改为`{}`初始化,比如`int a = 5;` 需要改为`int a{5};`
   - 变量初始化的时候, 如果是无符号整型需要添加`U`或者`UL`后缀,比如: `unsigned int b=42;`需要改为`unsigned int b{42U};`
4. 全局 flag 如果制定为`output-json`的情况下,需要以 json 形式输出, 方便与其他工具集成

输出要求:
包含关键信息：变量/函数/类的名称，变量类型，变量所在行， 变量所在文件的绝对路径
