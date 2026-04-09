#!/usr/bin/env python3
import sys
import re

# -----------------------------
# Prefix Preprocessor
# -----------------------------
C_TYPES = r'void|int|char|float|double|struct\s+\w+|enum\s+\w+|typedef\s+\w+'

def reindent_code(code):
    lines = code.splitlines()
    indented = []
    indent_level = 0
    for line in lines:
        stripped = line.strip()
        if stripped == "{":
            indented.append("    " * indent_level + "{")
            indent_level += 1
        elif stripped in ("}", "};", "} ;"):
            indent_level -= 1
            indented.append("    " * indent_level + stripped)
        elif stripped:
            indented.append("    " * indent_level + stripped)
        else:
            indented.append("")
    return "\n".join(indented)

# -----------------------------
# Typestruct Preprocessor
# -----------------------------
def process_typestruct(code):
    lines = code.splitlines()
    output_lines = []

    # Patterns
    typestruct_inline_pattern = re.compile(r'^\s*typestruct\s+([A-Za-z_]\w*)\s*\{')
    typestruct_start_pattern = re.compile(r'^\s*typestruct\s+([A-Za-z_]\w*)\s*$')
    typestruct_decl_pattern = re.compile(r'^\s*typestruct\s+([A-Za-z_]\w*)\s*;\s*$')

    current_struct = None
    brace_depth = 0
    waiting_for_brace = False
    pending_struct_name = None

    emitted_typedefs = set()

    for line in lines:
        stripped = line.strip()

        # --- CASE 1: Not inside struct ---
        if current_struct is None and not waiting_for_brace:

            # Inline: typestruct Foo {
            inline_match = typestruct_inline_pattern.match(line)
            if inline_match:
                struct_name = inline_match.group(1)
                current_struct = struct_name
                brace_depth = 1
                output_lines.append(line.replace("typestruct", "struct", 1))
                continue

            # Split start: typestruct Foo
            start_match = typestruct_start_pattern.match(line)
            if start_match:
                pending_struct_name = start_match.group(1)
                waiting_for_brace = True
                output_lines.append(line.replace("typestruct", "struct", 1))
                continue

            # ❌ Forward declarations forbidden
            decl_match = typestruct_decl_pattern.match(line)
            if decl_match:
                struct_name = decl_match.group(1)
                raise ValueError(
                    f"Forward declarations using 'typestruct' are not allowed: '{struct_name}'"
                )

            output_lines.append(line)
            continue

        # --- CASE 2: Waiting for opening brace ---
        if waiting_for_brace:
            if "{" in line:
                current_struct = pending_struct_name
                pending_struct_name = None
                waiting_for_brace = False
                brace_depth = line.count('{') - line.count('}')
                output_lines.append(line)
                continue
            else:
                output_lines.append(line)
                continue

        # --- CASE 3: Inside struct body ---
        brace_depth += line.count('{') - line.count('}')
        output_lines.append(line)

        if brace_depth == 0:
            typedef_line = f"typedef struct {current_struct} {current_struct};"
            if typedef_line not in emitted_typedefs:
                output_lines.append(typedef_line)
                emitted_typedefs.add(typedef_line)
            current_struct = None

    return "\n".join(output_lines)

# -----------------------------
# Prefix Handling
# -----------------------------
def process_prefixes(code):
    prefix_stack = []
    output_lines = []
    block_stack = []
    expect_prefix_brace = False
    defined_identifiers = {}

    prefix_open_pattern = re.compile(r'\s*prefix\s+(\w+)\s*')
    def_pattern = re.compile(r'([\w\*\s]+?\s+)(\w+)\s*(\([^;{]*\))')
    array_def_pattern = re.compile(r'([\w\*\s]+?\s+)(\w+)(\s*\[[^\]]*\])(\s*(?:=\s*[^;]+)?)?\s*;')
    var_def_pattern = re.compile(r'([\w\*\s]+?\s+)(\w+)(\s*(?:=\s*[^;]+)?)?\s*;')
    usage_pattern = re.compile(r'(?<!\w)(::)?((?:\w+::)*)(\w+)\(')
    identifier_pattern = re.compile(r'(?<![\w:])(::)?((?:\w+::)*)(\w+)(?!\s*\()')
    typedef_pattern = re.compile(r'\btypedef\s+struct\s+(\w+)\s+(\w+)\s*;')
    struct_inline_pattern = re.compile(r'\bstruct\s+(\w+)\s*\{')
    struct_decl_pattern = re.compile(r'\bstruct\s+(\w+)\s*$')

    def replace_usage(m):
        leading_colons, ns_chain, func = m.groups()
        ns_chain = ns_chain.rstrip("::") if ns_chain else ""
        prefixes = ns_chain.split("::") if ns_chain else []

        if leading_colons:
            if prefixes:
                prefix = "__".join(prefixes)
            else:
                prefix = ""
            return f"{prefix}__{func}(" if prefix else f"{func}("

        if func in defined_identifiers:
            mangled = defined_identifiers[func]
            if prefix_stack and mangled.startswith("__".join(prefix_stack) + "__"):
                return mangled + "("

        full_prefix = prefix_stack + prefixes
        prefix = "__".join(full_prefix) if full_prefix else ""
        return f"{prefix}__{func}(" if prefix else f"{func}("

    def replace_identifier(m):
        leading_colons, ns_chain, name = m.groups()
        ns_chain = ns_chain.rstrip("::") if ns_chain else ""
        prefixes = ns_chain.split("::") if ns_chain else []

        if leading_colons:
            if prefixes:
                prefix = "__".join(prefixes)
                return f"{prefix}__{name}"
            return name

        if prefixes:
            prefix = "__".join(prefixes)
            return f"{prefix}__{name}"

        if name in defined_identifiers:
            mangled = defined_identifiers[name]
            if prefix_stack and mangled.startswith("__".join(prefix_stack) + "__"):
                return mangled

        return name

    for line in code.splitlines():
        stripped = line.strip()
        prefix_open = prefix_open_pattern.match(line)
        if prefix_open:
            prefix_stack.append(prefix_open.group(1))
            expect_prefix_brace = True
            continue

        if stripped == "{" and expect_prefix_brace:
            expect_prefix_brace = False
            block_stack.append("prefix")
            continue

        if stripped == "{" and not expect_prefix_brace:
            output_lines.append(line)
            block_stack.append("block")
            continue

        if stripped in ("};", "} ;"):
            if block_stack:
                block_stack.pop()
            output_lines.append(line)
            continue

        if stripped == "}":
            if block_stack and block_stack[-1] == "prefix":
                block_stack.pop()
                if prefix_stack:
                    prefix_stack.pop()
                continue
            if block_stack:
                block_stack.pop()
            output_lines.append(line)
            continue

        processed_line = line
        is_prefix_scope = bool(prefix_stack and block_stack and block_stack[-1] == "prefix")
        if is_prefix_scope:
            current_prefix = "__".join(prefix_stack)
            struct_inline_match = struct_inline_pattern.search(line)
            struct_decl_match = struct_decl_pattern.search(line)
            if struct_inline_match or struct_decl_match:
                struct_name = struct_inline_match.group(1) if struct_inline_match else struct_decl_match.group(1)
                mangled_name = f"{current_prefix}__{struct_name}"
                defined_identifiers[struct_name] = mangled_name
                if struct_inline_match:
                    processed_line = struct_inline_pattern.sub(lambda m: f"struct {mangled_name}{{", line)
                else:
                    processed_line = struct_decl_pattern.sub(lambda m: f"struct {mangled_name}", line)
            else:
                typedef_match = typedef_pattern.search(line)
                if typedef_match:
                    struct_name, typedef_name = typedef_match.groups()
                    mangled_struct = f"{current_prefix}__{struct_name}"
                    mangled_typedef = f"{current_prefix}__{typedef_name}"
                    defined_identifiers[struct_name] = mangled_struct
                    defined_identifiers[typedef_name] = mangled_typedef
                    processed_line = f"typedef struct {mangled_struct} {mangled_typedef};"
                else:
                    def_match = def_pattern.search(line)
                    if def_match:
                        def replace_def(m):
                            decl, name, args = m.groups()
                            mangled_name = f"{current_prefix}__{name}"
                            defined_identifiers[name] = mangled_name
                            return f"{decl}{mangled_name}{args or ''}"
                        processed_line = def_pattern.sub(replace_def, line)
                    else:
                        array_match = array_def_pattern.search(line)
                        if array_match:
                            decl, name, array_part, init = array_match.groups()
                            mangled_name = f"{current_prefix}__{name}"
                            defined_identifiers[name] = mangled_name
                            processed_line = array_def_pattern.sub(
                                lambda m: f"{decl}{mangled_name}{array_part}{init or ''};",
                                line,
                            )
                            processed_line = identifier_pattern.sub(replace_identifier, processed_line)
                        else:
                            var_match = var_def_pattern.search(line)
                            if var_match:
                                decl, name, init = var_match.groups()
                                mangled_name = f"{current_prefix}__{name}"
                                defined_identifiers[name] = mangled_name
                                processed_line = var_def_pattern.sub(
                                    lambda m: f"{decl}{mangled_name}{init};",
                                    line,
                                )
                                processed_line = identifier_pattern.sub(replace_identifier, processed_line)
                            else:
                                processed_line = usage_pattern.sub(replace_usage, line)
                                processed_line = identifier_pattern.sub(replace_identifier, processed_line)
        else:
            processed_line = usage_pattern.sub(replace_usage, line)
            processed_line = identifier_pattern.sub(replace_identifier, processed_line)

        output_lines.append(processed_line)

    raw_code = "\n".join(output_lines)
    return reindent_code(raw_code)

# -----------------------------
# Indef Handling
# -----------------------------
indef_pattern = re.compile(
    r'^\s*indef\s+([^\n=]+?)\s+([A-Za-z_]\w*)\s*=\s*(.+?);$',
    re.MULTILINE,
)

def process_indef(code):
    function_start_pattern = re.compile(
        r'^\s*[\w\*\s]+?\s+([A-Za-z_]\w*)\s*\([^;{]*\)\s*(\{)?\s*$'
    )
    lines = code.splitlines()
    output_lines = []
    current_function = None
    current_function_start = None
    pending_function = None
    brace_depth = 0
    pending_inserts = {}

    def make_define(name, type_text, expr, prefix=None):
        if prefix:
            name = f"{prefix}__{name}"
        return f"#define {name} ({type_text})({expr})"

    for line in lines:
        stripped = line.strip()
        if current_function is None and pending_function is None:
            indef_match = indef_pattern.match(line)
            if indef_match:
                declaration = indef_match.group(0)
                type_text = indef_match.group(1).strip()
                name = indef_match.group(2)
                expr = indef_match.group(3).strip()
                if re.search(r'\b(static|mut|ref)\b', declaration):
                    raise ValueError("indef declaration cannot contain static, mut, or ref")
                output_lines.append(make_define(name, type_text, expr))
                continue

            func_match = function_start_pattern.match(line)
            if func_match:
                pending_function = func_match.group(1)
                current_function_start = len(output_lines)
                output_lines.append(line)
                if func_match.group(2) == '{':
                    current_function = pending_function
                    pending_function = None
                    brace_depth = 1
                    pending_inserts[current_function] = []
                continue

            output_lines.append(line)
            continue

        if current_function is None and pending_function is not None:
            if stripped == '{':
                current_function = pending_function
                pending_function = None
                brace_depth = 1
                pending_inserts[current_function] = []
                output_lines.append(line)
                continue
            func_match = function_start_pattern.match(line)
            if func_match:
                output_lines.append(line)
                continue
            output_lines.append(line)
            continue

        if current_function is not None:
            indef_match = indef_pattern.match(line)
            if indef_match:
                declaration = indef_match.group(0)
                type_text = indef_match.group(1).strip()
                name = indef_match.group(2)
                expr = indef_match.group(3).strip()
                if re.search(r'\b(static|mut|ref)\b', declaration):
                    raise ValueError("indef declaration cannot contain static, mut, or ref")
                pending_inserts[current_function].append(
                    make_define(name, type_text, expr, prefix=current_function)
                )
                continue

            open_count = line.count('{')
            close_count = line.count('}')
            brace_depth += open_count - close_count
            output_lines.append(line)
            if brace_depth == 0:
                inserts = pending_inserts.get(current_function, [])
                if inserts:
                    output_lines[current_function_start:current_function_start] = inserts
                current_function = None
                current_function_start = None
                brace_depth = 0
            continue

        output_lines.append(line)

    return '\n'.join(output_lines)

# -----------------------------
# Mut / Const Handling
# -----------------------------
def extract_c_types(code: str):
    # --- Define qualifiers to remove ---
    qualifiers = {
        "const", "volatile", "static", "extern", "register", "restrict"
    }

    # --- Cleanup ---
    # Remove qualifiers
    for q in qualifiers:
        code = re.sub(rf'\b{q}\b', '', code)

    # Remove mut/safe/cleanpop keywords
    code = re.sub(r'\b(mut|safe)\b|\bcleanpop\s*(\([^)]*\))?', '', code)

    # Remove preprocessor lines
    code = re.sub(r'^.*#.*$', '', code, flags=re.MULTILINE)

    # Remove comments
    code = re.sub(r"//.*?$|/\*.*?\*/", "", code, flags=re.S | re.M)

    # Remove string and character literals
    code = re.sub(r'"(\\.|[^"\\])*"|\'(\\.|[^\'\\])*\'', '', code)

    # --- Regex to match variable declarations ---
    # Matches: TYPE name;   or  TYPE name = ...
    # Captures TYPE as group 1
    pattern = re.compile(
        r'\b([A-Za-z_]\w*(?:\s*\*)*)\s+[A-Za-z_]\w*\s*(?:=[^;]*)?;', re.M
    )

    types = set()
    for match in pattern.finditer(code):
        type_name = match.group(1).strip()
        # Remove trailing '*' from pointers
        type_name = type_name.replace('*', '').strip()
        if type_name:
            types.add(type_name)

    return types

def add_const_to_type(code: str, type_name: str) -> str:
    t = re.escape(type_name)

    # -------------------------
    # Step 0: Protect typedef lines
    # -------------------------
    typedef_pattern = re.compile(r'^.*\btypedef\b.*$', re.MULTILINE)
    typedef_protected = []

    def protect_typedef(match):
        typedef_protected.append(match.group(0))
        return f"__TYPEDEF_{len(typedef_protected)-1}__"

    code = typedef_pattern.sub(protect_typedef, code)

    # -------------------------
    # Step 1: Protect struct definitions
    # -------------------------
    struct_def_pattern = re.compile(
        r'struct\s+\w+\s*\{.*?\};',
        re.DOTALL
    )
    struct_protected = []

    def protect_struct(match):
        struct_protected.append(match.group(0))
        return f"__STRUCT_{len(struct_protected)-1}__"

    code = struct_def_pattern.sub(protect_struct, code)

    # -------------------------
    # Step 2: Add const to types and struct usages
    # -------------------------
    add_const_pattern = re.compile(
        rf'''
        (?<!\w)
        (
            (?:struct\s+)?       # optional struct usage
            {t}\b
            (?:\s*\*+)?          # optional pointer(s)
        )
        (?!\s*\{{)               # not start of struct definition
        (?!\s+{t}\b)             # not type type
        ''',
        re.VERBOSE | re.MULTILINE
    )

    code = add_const_pattern.sub(r'const \1', code)

    # -------------------------
    # Step 3: Remove mut const
    # -------------------------
    code = re.sub(r'\bmut\s+const\s+', '', code)

    # -------------------------
    # Step 4: Collapse const const
    # -------------------------
    code = re.sub(r'\bconst\s+const\b', 'const', code)

    # -------------------------
    # Step 5: Add const after pointers (only if next char is whitespace or *)
    # -------------------------
    pointer_pattern = re.compile(r'\*(?=\s|\*)')
    code = pointer_pattern.sub(r'* const', code)

    # -------------------------
    # Step 6: Collapse any new const const
    # -------------------------
    code = re.sub(r'\bconst\s+const\b', 'const', code)

    # -------------------------
    # Step 7: Remove const mut sequences
    # -------------------------
    code = re.sub(r'\bconst\s+mut\b', '', code)

    # -------------------------
    # Step 8: Remove const from return
    # -------------------------
    code = re.sub(r'\bconst\s+return\b', 'return', code)

    # -------------------------
    # Step 9: Restore protected structs
    # -------------------------
    def restore_struct(match):
        index = int(match.group(1))
        return struct_protected[index]

    code = re.sub(r'__STRUCT_(\d+)__', restore_struct, code)

    # -------------------------
    # Step 10: Restore typedefs
    # -------------------------
    def restore_typedef(match):
        index = int(match.group(1))
        return typedef_protected[index]

    code = re.sub(r'__TYPEDEF_(\d+)__', restore_typedef, code)

    # -------------------------
    # Step 11: Remove const from struct forward declarations
    # -------------------------
    code = re.sub(r'\bconst\s+struct\s+([A-Za-z0-9_]+)\s*;', r'struct \1;', code)

    return code

def process_mut_const(code):
    types = extract_c_types(code)
    for t in types:
        code = add_const_to_type(code, t)

    return code

# -----------------------------
# Process safe pointers
# -----------------------------
def validate_safe_functions(code):
    """
    Validates safe functions in C code.

    Rules enforced:
    1. Safe consistency between declaration and definition.
    2. Safe functions must return pointer types.
    3. All return statements in safe functions must be 'return <identifier>;'
       with only letters, numbers, underscore.
    4. Returned variable must be declared safe (argument or local variable).
    """

    # -------------------------
    # Patterns
    # -------------------------
    definition_pattern = re.compile(
        r'^\s*'
        r'(?P<safe>safe\s+)?'
        r'(?P<ret>[A-Za-z_][\w\s\*\d]*?)\s+'
        r'(?P<name>[A-Za-z_][A-Za-z0-9_]*)'
        r'\s*\((?P<params>[^)]*)\)\s*'
        r'\{(?P<body>.*?)\}',
        re.MULTILINE | re.DOTALL
    )

    declaration_pattern = re.compile(
        r'^\s*'
        r'(?P<safe>safe\s+)?'
        r'(?P<ret>[A-Za-z_][\w\s\*\d]*?)\s+'
        r'(?P<name>[A-Za-z_][A-Za-z0-9_]*)'
        r'\s*\((?P<params>[^)]*)\)\s*;',
        re.MULTILINE
    )

    return_pattern = re.compile(r'\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*;')

    # -------------------------
    # Robust variable declaration pattern
    # -------------------------
    var_decl_pattern = re.compile(
        r'^(?P<prefix>(?:\b\w+\b\s+)*)'  # all keywords before variable name
        r'(?:[A-Za-z_][A-Za-z0-9_\s\*]*)\s+'  # type portion (int*, mut int*, etc.)
        r'(?P<var>[A-Za-z_][A-Za-z0-9_]*)'     # variable name
        r'\s*(?:=.*)?;',                        # optional initialization
        re.MULTILINE
    )

    declarations = {}
    definitions = {}

    # -------------------------
    # Parse declarations
    # -------------------------
    for match in declaration_pattern.finditer(code):
        name = match.group("name")
        is_safe = bool(match.group("safe"))
        ret_type = match.group("ret").strip()

        if is_safe and '*' not in ret_type:
            raise ValueError(
                f"Function '{name}' is marked safe but return type is not a pointer: '{ret_type}'"
            )

        declarations[name] = {"safe": is_safe, "return_type": ret_type}

    # -------------------------
    # Parse definitions
    # -------------------------
    for match in definition_pattern.finditer(code):
        name = match.group("name")
        is_safe = bool(match.group("safe"))
        ret_type = match.group("ret").strip()
        body = match.group("body")

        if is_safe and '*' not in ret_type:
            raise ValueError(
                f"Function '{name}' is marked safe but return type is not a pointer: '{ret_type}'"
            )

        # -------------------------
        # Parse arguments
        # -------------------------
        safe_args = set()
        args = [p.strip() for p in match.group("params").split(",") if p.strip()]
        for arg in args:
            tokens = re.findall(r'[A-Za-z_][A-Za-z0-9_]*', arg)
            if not tokens:
                continue
            var_name = tokens[-1]
            if arg.startswith("safe"):
                safe_args.add(var_name)

        # -------------------------
        # Collect all safe local variables
        # -------------------------
        body_lines = body.splitlines()
        safe_locals = set()
        for line in body_lines:
            line = line.strip()
            decl_match = var_decl_pattern.match(line)
            if decl_match:
                var_name = decl_match.group("var")
                prefix_keywords = decl_match.group("prefix").split()
                if "safe" in prefix_keywords:
                    safe_locals.add(var_name)

        # -------------------------
        # Validate return statements
        # -------------------------
        if is_safe:
            for ret_match in return_pattern.finditer(body):
                ret_var = ret_match.group(1)

                # Must be a single identifier
                if not re.fullmatch(r'[A-Za-z0-9_]+', ret_var):
                    raise ValueError(
                        f"Function '{name}' return '{ret_var}' is invalid, must be a single identifier"
                    )

                # Must be declared safe
                if ret_var not in safe_locals and ret_var not in safe_args:
                    raise ValueError(
                        f"Function '{name}' returns '{ret_var}' which is not safe "
                        f"(not a safe argument or safe local)"
                    )

        definitions[name] = {"safe": is_safe, "return_type": ret_type}

    # -------------------------
    # Safe consistency check
    # -------------------------
    for name, decl in declarations.items():
        if name in definitions:
            defn = definitions[name]
            if decl["safe"] != defn["safe"]:
                raise ValueError(
                    f"Function '{name}' mismatch: declaration has "
                    f"{'safe' if decl['safe'] else 'no safe'}, "
                    f"but definition has {'safe' if defn['safe'] else 'no safe'}"
                )

    return {"declarations": declarations, "definitions": definitions}

def add_safe_nullchecks_locals(code):
    lines = code.splitlines()
    output = []

    pattern = re.compile(
        r'^\s*'
        r'(?P<prefix>safe\s+)?'            
        r'(?P<type>[\w\s]+?)'              
        r'\s*\*\s*'
        r'(?P<const_after>\bconst\b\s*)?'
        r'(?P<var>[A-Za-z_][A-Za-z0-9_]*)' 
        r'(?:\s*=\s*[^;]+)?'               
        r'\s*;'
    )

    for line in lines:
        match = pattern.match(line)
        if not match:
            output.append(line)
            continue

        prefix = match.group('prefix') or ""
        var_name = match.group('var')
        const_after = match.group('const_after')

        if not prefix:
            output.append(line)
            continue

        if not const_after:
            raise ValueError(f"Safe pointer must have const before variable: {var_name}")

        output.append(line)
        indent = re.match(r'^\s*', line).group(0)
        output.append(f"{indent}EC__NULL__CHECK({var_name});")

    return "\n".join(output)


def add_safe_nullchecks_params(code):
    lines = code.splitlines()
    output = []

    i = 0
    while i < len(lines):
        line = lines[i]

        if '(' not in line:
            output.append(line)
            i += 1
            continue

        # Collect all lines of function signature until '{'
        func_lines = []
        while i < len(lines):
            func_lines.append(lines[i])
            if '{' in lines[i]:
                i += 1
                break
            i += 1

        # Extract the parameters string
        func_text = ' '.join(func_lines)
        params_match = re.search(r'\((.*)\)\s*\{', func_text)
        if not params_match:
            output.extend(func_lines)
            continue

        params_str = params_match.group(1)
        param_list = [p.strip() for p in params_str.split(',') if p.strip()]

        # Detect all safe pointer variables
        vars_to_check = []
        param_pattern = re.compile(
            r'(?P<prefix>safe\s+)?'
            r'(?P<type>[\w\s]+?)'
            r'\s*\*\s*'
            r'(?P<const_after>\bconst\b\s*)?'
            r'(?P<var>[A-Za-z_][A-Za-z0-9_]*)'
        )
        for p in param_list:
            for m in param_pattern.finditer(p):
                prefix = m.group('prefix') or ""
                var_name = m.group('var')
                const_after = m.group('const_after')
                if prefix and not const_after:
                    raise ValueError(f"Safe pointer parameter must have const: {var_name}")
                if prefix:
                    vars_to_check.append(var_name)

        # Add original function lines
        output.extend(func_lines)

        # Add null checks at start of function body
        first_line_indent = re.match(r'^(\s*)', func_lines[-1]).group(1) + "    "
        for var in vars_to_check:
            output.append(f"{first_line_indent}EC__NULL__CHECK({var});")

    return "\n".join(output)

def has_safe_keyword(code):
    """
    Returns True if the keyword 'safe' is used anywhere in the code,
    otherwise False. Only matches 'safe' as a full word.
    """
    return bool(re.search(r'\bsafe\b', code))

def remove_safe_keyword(code):
    """
    Removes all instances of the keyword 'safe' from the code,
    preserving all spacing, indentation, and formatting.
    Only removes 'safe' as a full word.
    """
    # Use word boundaries to match 'safe' as a separate word
    cleaned_code = re.sub(r'\bsafe\b\s?', '', code)
    return cleaned_code

def find_last_include_line(code):
    """
    Finds the last line that starts with '#include'.
    Returns the 0-based line number of the last include,
    or -1 if no include lines are present.
    """
    last_include_index = -1
    lines = code.splitlines()
    
    for i, line in enumerate(lines):
        if line.strip().startswith("#include"):
            last_include_index = i
    
    return last_include_index

def insert_nullcheck_macro_after_line(line_number, code):
    """
    Inserts the EC__NULL__CHECK macro block immediately after the specified line number.
    Uses a safe macro that prints the file and line correctly.
    """
    macro_block = (
        "#ifndef EC__NULL__CHECK\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#define EC__NULL__CHECK(x) \\\n"
        "    do { \\\n"
        "        if ((x) == NULL) { \\\n"
        "            fprintf(stderr, \"File %s - Line %d had illegal null pointer, now exiting...\\n\", __FILE__, __LINE__); \\\n"
        "            exit(1); \\\n"
        "        } \\\n"
        "    } while(0)\n"
        "#endif"
    )

    lines = code.splitlines()
    
    # Insert after the specified line number
    insert_index = line_number + 1
    if insert_index > len(lines):
        insert_index = len(lines)  # append at end if line_number is last line

    # Insert the macro block as multiple lines
    new_lines = lines[:insert_index] + [""] + macro_block.splitlines() + lines[insert_index:]

    return "\n".join(new_lines)

def has_nullcheck_macro(code):
    """
    Returns True if the C code contains a definition of EC__NULL__CHECK macro,
    otherwise False.
    """
    # \s* allows optional spaces between #define and macro name
    pattern = re.compile(r'^\s*#define\s+EC__NULL__CHECK\b', re.MULTILINE)
    return bool(pattern.search(code))

def process_safe_pointers(code):
    validate_safe_functions(code)
    code = add_safe_nullchecks_locals(code)
    code = add_safe_nullchecks_params(code)
    if has_safe_keyword(code) and not has_nullcheck_macro(code):
        code = remove_safe_keyword(code)
        last_include_line = find_last_include_line(code)
        code = insert_nullcheck_macro_after_line(last_include_line, code)

    return code

# -----------------------------
# Add typenum for typesafe enums
# -----------------------------
def transform_typenum(code):
    """
    Transforms 'typenum' declarations into:
    - struct + typedef
    - #defines for each value
    - #define TypeName__count N

    Raises ValueError if:
    - any entry lacks '='
    - duplicate numeric values are found
    - duplicate names are found
    - no values exist
    """

    pattern = re.compile(
        r'typenum\s+([A-Za-z_][A-Za-z0-9_]*)\s*{(.*?)}\s*;',
        re.DOTALL
    )

    def replacer(match):
        type_name = match.group(1)
        body = match.group(2).strip()

        values = [v.strip() for v in body.split(',') if v.strip()]

        # 🚨 Empty typenum
        if len(values) == 0:
            raise ValueError(f"Typenum '{type_name}' has no values")

        defines = []
        seen_values = {}   # num -> name
        seen_names = set() # value names

        for v in values:
            if '=' not in v:
                raise ValueError(
                    f"Typenum '{type_name}' has invalid entry (missing '='): {v}"
                )

            name_part, value_part = v.split('=', 1)
            value_name = name_part.strip()
            num = value_part.strip()

            # 🚨 Duplicate name check
            if value_name in seen_names:
                raise ValueError(
                    f"Typenum '{type_name}' has duplicate name: '{value_name}'"
                )
            seen_names.add(value_name)

            # 🚨 Duplicate numeric value check
            if num in seen_values:
                raise ValueError(
                    f"Typenum '{type_name}' has duplicate value '{num}' "
                    f"for '{value_name}' and '{seen_values[num]}'"
                )
            seen_values[num] = value_name

            defines.append(
                f"#define {type_name}__{value_name} (({type_name}){{ .{type_name}_value = {num} }})"
            )

        count = len(values)

        struct_def = (
            f"struct {type_name} {{ int {type_name}_value; }};\n"
            f"typedef struct {type_name} {type_name};"
        )

        count_define = f"#define {type_name}__count {count}"
        type_equals = f"#define {type_name}__equals(a, b) ((a).{type_name}_value == (b).{type_name}_value)"
        type_get_value = f"#define {type_name}__get(a) ((a).{type_name}_value)"

        return (
            struct_def
            + "\n"
            + "\n".join(defines)
            + "\n"
            + count_define
            + "\n"
            + type_equals
            + "\n"
            + type_get_value
        )

    return pattern.sub(replacer, code)

def has_typenum_keyword(code):
    return bool(re.search(r'\btypenum\b', code))

def process_typenum(code):
    if has_typenum_keyword(code):
        code = transform_typenum(code)
    return code

# -----------------------------
# Add cleanpop management
# -----------------------------
def handle_cleanpop(code: str) -> str:
    lines = code.splitlines()
    result = []

    active_vars = []
    scope_level = 0
    scope_statements = {}

    for line in lines:
        # -------------------------
        # Extract comments
        # -------------------------
        comment_match = re.search(r'//.*|/\*.*?\*/', line)
        comment = ""
        if comment_match:
            # Ensure a space before the comment
            comment = " " + comment_match.group(0)
            # Remove comment temporarily
            code_line = line[:comment_match.start()]
        else:
            code_line = line

        stripped = code_line.strip()

        open_braces = code_line.count('{')
        close_braces = code_line.count('}')
        has_return = bool(re.search(r'\breturn\b', code_line))
        scope_statements.setdefault(scope_level, [])

        # -------------------------
        # CLEANPOP
        # -------------------------
        match = re.match(r'^(\s*)cleanpop(?:\(\))?\s+([^;]+);', code_line)
        if match:
            indent = match.group(1)
            body = match.group(2).strip()

            if '*' in body:
                raise ValueError(f"Invalid cleanpop (pointer not allowed): {line}")
            if '=' in body:
                raise ValueError(f"Invalid cleanpop (assignment not allowed): {line}")

            is_const = bool(re.search(r'\bconst\b', body))
            parts = body.split()
            if len(parts) < 2:
                raise ValueError(f"Invalid cleanpop: {line}")

            var_name = parts[-1]
            type_tokens = parts[:-1]
            type_no_const_tokens = [t for t in type_tokens if t != "const"]
            type_name_no_const = " ".join(type_no_const_tokens)
            type_name_full = " ".join(type_tokens)

            active_vars.append({
                "name": var_name,
                "type": type_name_no_const,
                "is_const": is_const,
                "scope": scope_level,
                "indent": indent
            })

            # Add variable declaration with comment
            result.append(f"{indent}{type_name_full} {var_name};{comment}")

            # Add populate call
            if is_const:
                result.append(f"{indent}{type_name_no_const}__populate(({type_name_no_const}*)&{var_name});")
            else:
                result.append(f"{indent}{type_name_no_const}__populate(&{var_name});")

            continue

        # -------------------------
        # RETURN handling
        # -------------------------
        if has_return:
            indent = re.match(r'^\s*', code_line).group(0)
            match_ret = re.search(r'\breturn\s+([A-Za-z_][A-Za-z0-9_]*)\s*;', code_line)
            if match_ret:
                ret_var = match_ret.group(1)
                for var in active_vars:
                    if var["name"] == ret_var:
                        raise ValueError(
                            f"Cannot return cleanpop variable '{ret_var}' (it will be cleaned up automatically)"
                        )

            # Insert cleanup statements before return
            for var in reversed(active_vars):
                if var["scope"] <= scope_level:
                    if var["is_const"]:
                        cleanup = f"{indent}{var['type']}__cleanup(({var['type']}*)&{var['name']});"
                    else:
                        cleanup = f"{indent}{var['type']}__cleanup(&{var['name']});"
                    result.append(cleanup)

            scope_statements[scope_level].append("return")

        # -------------------------
        # Normal line (add back comment with space)
        # -------------------------
        if stripped:
            result.append(code_line.rstrip() + comment)
        else:
            result.append(line)

        # -------------------------
        # Update scope
        # -------------------------
        scope_level += open_braces
        scope_statements.setdefault(scope_level, [])

        if close_braces > 0:
            for _ in range(close_braces):
                statements = scope_statements.get(scope_level, [])
                skip_cleanup = len(statements) > 0 and statements[-1] == "return"

                new_active = []
                for var in reversed(active_vars):
                    if var["scope"] == scope_level:
                        if skip_cleanup:
                            continue
                        indent = var["indent"]
                        if var["is_const"]:
                            cleanup = f"{indent}{var['type']}__cleanup(({var['type']}*)&{var['name']});"
                        else:
                            cleanup = f"{indent}{var['type']}__cleanup(&{var['name']});"
                        result.insert(len(result) - 1, cleanup)
                    else:
                        new_active.insert(0, var)
                active_vars = new_active
                scope_statements.pop(scope_level, None)
                scope_level -= 1

    return "\n".join(result)

def add_braces_to_if_else(code: str) -> str:
    lines = code.splitlines()
    result = []
    n = len(lines)
    i = 0
    in_macro = False  # Track multi-line macros

    while i < n:
        line = lines[i]
        stripped = line.strip()

        # Detect macro start
        if stripped.startswith('#define'):
            in_macro = True
            result.append(line)
            i += 1
            continue

        # Track continuation of macro
        if in_macro:
            result.append(line)
            if not stripped.endswith('\\'):
                in_macro = False
            i += 1
            continue

        # Match 'if (...)' or 'else'
        match = re.match(r'^(\s*)(if\s*\(.*?\)|else\b)(.*)', line)
        if match:
            indent = match.group(1)
            keyword = match.group(2)
            remainder = match.group(3).strip()

            # Already has braces on the same line? leave it
            if remainder.startswith('{'):
                result.append(line)
                i += 1
                continue

            # Lookahead: skip empty lines to check if next line starts with '{'
            j = i + 1
            next_line_is_block = False
            while j < n:
                next_line_stripped = lines[j].strip()
                if next_line_stripped == '':
                    j += 1
                    continue
                if next_line_stripped.startswith('{'):
                    next_line_is_block = True
                break

            # If there is already a block, do nothing
            if next_line_is_block:
                result.append(line)
                i += 1
                continue

            # Collect the statement (everything until the first semicolon)
            statement = remainder
            while ';' not in statement and i + 1 < n:
                i += 1
                statement += ' ' + lines[i].strip()
            statement = statement.rstrip(';').strip()

            # Wrap in braces
            result.append(f"{indent}{keyword} {{")
            result.append(f"{indent}    {statement};")
            result.append(f"{indent}}}")

            i += 1
            continue

        # Normal line
        result.append(line)
        i += 1

    return "\n".join(result)

def contains_cleanpop(code: str) -> bool:
    """
    Returns True if the code contains the word 'cleanpop' as a standalone keyword.
    """
    # \b ensures cleanpop is a full word, not part of another identifier
    pattern = re.compile(r'\bcleanpop\b')
    return bool(pattern.search(code))

def process_cleanpop(code):
    if contains_cleanpop(code):
        code = add_braces_to_if_else(code)
        code = handle_cleanpop(code)
    return code

# -----------------------------
# Generated code warning
# -----------------------------
def add_generated_warning(code: str) -> str:
    warning = (
        "// ===================================================\n"
        "// === WARNING! DO NOT EDIT THIS FILE! ===============\n"
        "// === This code was generated by EasyC (Prototype) ==\n"
        "// === and transpiled from an input file =============\n"
        "// ===================================================\n\n"
    )

    return warning + code

# -----------------------------
# Convert EasyC headers and files to C in include statements
# -----------------------------
def normalize_includes(code: str) -> str:
    """
    Rewrites #include paths:
    - .eh -> .h
    - .ec -> .c
    """

    include_pattern = re.compile(
        r'(#include\s*[<"])([^">]+)([>"])'
    )

    def replace(match):
        prefix = match.group(1)
        path = match.group(2)
        suffix = match.group(3)

        # Replace extensions
        if path.endswith(".eh"):
            path = path[:-3] + ".h"
        elif path.endswith(".ec"):
            path = path[:-3] + ".c"

        return f"{prefix}{path}{suffix}"

    return include_pattern.sub(replace, code)

# -----------------------------
# Clean up superflous const return types
# -----------------------------
def remove_const_from_non_pointer_returns(code: str) -> str:
    """
    Removes 'const' from function return types if the return type
    does NOT contain a pointer (*). Supports standard C qualifiers
    like static, inline, extern, etc.
    """

    pattern = re.compile(
        r'(^\s*)'                                      # indentation
        r'(?P<ret>(?:\b\w+\b[\s\*]+)*?)'                # return type + qualifiers
        r'(?P<name>[A-Za-z_][A-Za-z0-9_]*)'             # function name
        r'\s*\((?P<params>[^)]*)\)'                     # parameters
        r'(?P<ending>\s*[;{])',                        # ; or {
        re.MULTILINE
    )

    def replacer(match):
        indent = match.group(1)
        ret = match.group("ret")
        name = match.group("name")
        params = match.group("params")
        ending = match.group("ending")

        # If NO pointer in return type → remove 'const'
        if '*' not in ret:
            ret = re.sub(r'\bconst\b\s*', '', ret)

        # Clean up extra spaces
        ret = re.sub(r'\s+', ' ', ret).strip()

        return f"{indent}{ret} {name}({params}){ending}"

    return pattern.sub(replacer, code)

# -----------------------------
# Make indentation consistent (4 spaces)
# -----------------------------
def standardize_indentation(code: str, spaces_per_level: int = 4) -> str:
    """
    Standardizes C indentation to 4 spaces per block.
    Ignores braces inside strings and comments.
    Handles cases like '} else {' correctly.
    """

    def strip_strings_and_comments(s):
        # Remove // comments
        s = re.sub(r'//.*', '', s)

        # Remove /* */ comments (single-line safe)
        s = re.sub(r'/\*.*?\*/', '', s)

        # Remove string literals
        s = re.sub(r'"(\\.|[^"\\])*"', '', s)

        # Remove char literals
        s = re.sub(r"'(\\.|[^'\\])+'", '', s)

        return s

    lines = code.splitlines()
    result = []
    indent_level = 0

    for line in lines:
        stripped = line.strip()

        if not stripped:
            result.append('')
            continue

        # Remove strings/comments before counting braces
        clean = strip_strings_and_comments(stripped)

        # Count leading closing braces
        leading_closes = len(re.match(r'^}+', clean).group(0)) if re.match(r'^}+', clean) else 0

        # Step 1: reduce indent for leading '}'
        indent_level = max(indent_level - leading_closes, 0)

        # Step 2: write line
        result.append(' ' * (indent_level * spaces_per_level) + stripped)

        # Step 3: count remaining braces
        open_braces = clean.count('{')
        close_braces = clean.count('}')

        # subtract the ones already handled at start
        close_braces -= leading_closes

        indent_level += open_braces - close_braces

        if indent_level < 0:
            indent_level = 0

    return '\n'.join(result)

# -----------------------------
# Fix if(...) without space
# -----------------------------
def fix_if_spacing(code: str) -> str:
    """
    Ensure there is a space between 'if' and '(' in C code.
    Converts 'if(...)' → 'if (...)'.
    """
    # Pattern: match 'if' followed immediately by '(' (not preceded by other letters)
    pattern = re.compile(r'\bif\(')
    # Replace with 'if ('
    fixed_code = pattern.sub('if (', code)
    return fixed_code

# -----------------------------
# Transpile
# -----------------------------
def transpile_ec(code):

    # Process EasyC keywords
    code = process_typenum(code)
    code = process_typestruct(code)
    code = process_prefixes(code)
    code = process_indef(code)
    code = process_mut_const(code)
    code = process_safe_pointers(code)
    code = process_cleanpop(code)

    # Cleanup and formatting
    code = add_generated_warning(code)
    code = normalize_includes(code)
    code = remove_const_from_non_pointer_returns(code)
    code = standardize_indentation(code)
    code = fix_if_spacing(code)

    return code

# -----------------------------
# CLI
# -----------------------------
if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python transpiler.py input.c output.c")
        sys.exit(1)

    infile, outfile = sys.argv[1], sys.argv[2]
    with open(infile) as f:
        code = f.read()

    try:
        result = transpile_ec(code)
    except Exception as e:
        print("Error:", e)
        sys.exit(1)

    with open(outfile, "w") as f:
        f.write(result)

    print(f"Done! EasyC transpiled: {outfile}")