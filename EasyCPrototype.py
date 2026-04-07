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
def preprocess_typestruct(code):
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
def preprocess_prefixes(code):
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

    # Remove mut/ref keywords
    code = re.sub(r'\b(mut|ref)\b', '', code)

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

def check_ref_mut(code: str):
    """
    Raises an error if the code contains 'ref mut'.
    """
    # Remove comments and strings to avoid false positives
    code_clean = re.sub(r"//.*?$|/\*.*?\*/", "", code, flags=re.S | re.M)
    code_clean = re.sub(r'"(\\.|[^"\\])*"|\'(\\.|[^\'\\])*\'', '', code_clean)

    # Search for 'ref' followed by 'mut' as whole words
    if re.search(r'\bref\b\s+\bmut\b', code_clean):
        raise ValueError("Error: 'ref mut' not allowed, change to 'mut ref'.")

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
    check_ref_mut(code)

    types = extract_c_types(code)
    print(len(types), "unique C types detected:")
    for t in types:
        print(t)
        code = add_const_to_type(code, t)

    return code

# -----------------------------
# Nullable Handling
# -----------------------------
def process_nullable(code):
    return code

# -----------------------------
# Transpile
# -----------------------------
def transpile_ec(code):
    # 1. Preprocess typestructs
    code = preprocess_typestruct(code)
    # 2. Process prefixes
    code = preprocess_prefixes(code)
    # 3. Process indef
    code = process_indef(code)
    # 4. Process mut/const
    code = process_mut_const(code)

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