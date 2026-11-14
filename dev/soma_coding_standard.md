# SOMA Framework Coding Standard

## Scope

- This standard applies to all source and header files in the SOMA library, except third-party files and SOMA files dedicated to integrating with third-party code. Unit tests and test fixtures are not required to follow this standard or the C++ rules.
- Apply it when creating or modifying in-scope code. Do not reformat unrelated files.
- Treat the rules below as mandatory except where a section identifies them as review prompts. If following a rule would prevent the project from building, follow mandatory repository, build, or toolchain constraints instead. Where this standard is silent, preserve project-specific requirements and established local conventions. Report conflicts and use the closest compatible implementation.
- This document owns general coding practices and SOMA-wide formatting and naming conventions. Consult [C++ Agent Rules](soma-cpp-rules.md) for C++-specific requirements and review guidance. Apply both in context and avoid unrelated changes; this standard and project and toolchain constraints take precedence if they conflict.

## Portability

- Keep SOMA library code platform- and compiler-agnostic. Avoid platform-specific or compiler-specific dependencies and constructs.

## Code Formatting

- Format in-scope source and header files you create or modify with Uncrustify using the repository’s `dev/uncrustify.cfg`. Do not reformat unrelated or excluded files.
- Put each function parameter in declaration and definition on a separate line.
- Use `//` comments; do not use block-comment syntax (`/* ... */`).

## Naming Convention

- Use English for source file names and identifiers.
- Name source files in lowercase words separated by underscores, such as `sensor_driver.cpp`.
- Use underscore-separated identifiers. Use lowercase for ordinary identifiers. Begin class, struct, and type-alias names with an uppercase letter.
- Write abbreviations in uppercase, such as `GNU`.
- Do not shadow variables declared in an outer scope.
- Begin Boolean variable names with a verb, usually `is_` (for example, `is_ready`).
- Use these prefixes and suffixes where applicable:

  | Use                                                                    | Convention                              |
  | ---------------------------------------------------------------------- | --------------------------------------- |
  | Pointer variable                                                       | `p_`                                    |
  | Constant variable                                                      | `c_`                                    |
  | Output parameter                                                       | `out_`                                  |
  | Class member variable                                                  | `m_`                                    |
  | Constant class member variable                                         | `mc_`                                   |
  | Pointer class member variable                                          | `mp_`                                   |
  | Global variable defined in a `.cpp` file                               | `g_`                                    |
  | Static variable                                                        | `s_`, `s_m_`, `s_mp_`                   |
  | Interface class                                                        | `I_`                                    |
  | Non-interface base class intended for derivation                       | `B_`                                    |
  | `struct` type                                                          | `_t`                                    |
  | Type alias declared with `using` except those that denote a class type | `_t`                                    |
  | `enum` type                                                            | `_e`                                    |
  | Value carrying a physical unit                                         | e.g. `_ms`, `_ticks`, `_us`, or `_degC` |
  | Local variable storing a function’s return value                       | `rv_`, `rv_p_data`                      |

- Name text-output modifier classes in lowercase, including `min_width`, `fill_with`, `precision`, `bytes`, `fraction`, and `memory_dump`. This is an exception to the usual uppercase class-name rule.
- Start member accessor names directly with the property or action; do not prefix them with `get_`.

## Code Documentation

- Write comments in English.
- Use comments to explain intent, constraints, or non-obvious behavior. Do not restate what the code or its identifiers already make clear; prefer precise names where possible.
- Do not leave commented-out code. Include code in comments only as part of an example or explanatory sample.
- If a block is intentionally empty, put `// intentionally empty` inside it.
- Always document exceptions from the the coding standard.

## Coding

- Do not add dead or unreachable code.
- Every `TODO` or `FIXME` comment must identify the work item or ticket that will address it.
- Parenthesize expressions so readers never need to infer operator precedence.
- Declare each variable in the narrowest scope in which it is used, and no earlier than necessary.
- Keep side effects out of expressions. Call setters or other state-changing functions as separate statements.
- Avoid magic numbers. Exceptions include familiar conversion factors, such as 1024 bytes per kilobyte or 60 seconds per minute.
- Include a final `else` in an `if` / `else if` chain when an unhandled case needs explicit action. A standalone `if` or a chain that intentionally does nothing for other cases does not require an empty `else`.
- Treat all compiler and linker warnings as errors.
- Prefer a clear return path. Early returns are acceptable for guard clauses or when they simplify control flow and preserve resource cleanup.
- Prefer one primary class declaration per header. Closely related support classes may share a header when keeping them together makes the API more convenient and cohesive, as with the text-output modifiers in `text_output.h`.

## Correctness and resource management

- Closed resources should not be accessed
- Zero should not be a possible denominator
- Memory access should be explicitly bounded to prevent buffer overflows
- Prevent signed integer overflow and unintended unsigned wraparound. Document intentional unsigned wraparound when its purpose is not obvious.
- Parameter values should be appropriate
- Member variables should be initialized
- Variables should be initialized before use
- Function exit paths should have appropriate return values
- Variables should not be self-assigned
- Non-empty statements should change control flow or have at least one side-effect
- Identical expressions should not be used on both sides of a binary operator
- Memory locations should not be released more than once
- Freed memory should not be used
- Related "if/else if" statements should not have the same condition
- Two branches in a conditional structure should not have exactly the same implementation
- Recursion should not be infinite
- Blocking functions should not be called inside critical sections
- Prefer scoped mutex guards where available; otherwise ensure every successful acquisition is released on all applicable paths.
- Parameters should be passed in the correct order
- When acquiring multiple mutexes, prevent deadlock by using a consistent lock order or a suitable coordinated locking mechanism. Document the lock order where it is not obvious.

## Design and maintenance

Use the following as review prompts. Preserve required interfaces and behavior, and allow intentional empty methods when needed.

- Use the correct deallocation method, release owned dynamic memory, and never free stack or non-owned memory.
- Unused function parameters should be removed
- Inheritance tree of classes should not be too deep
- Unused assignments should be removed
- Deprecated code should be removed
- Unused functions and methods should be removed
- Unused local variables should be removed
- Classes should not contain both public and private data members
- Methods should not be empty
- Classes should not have too many methods
- Structures should not have too many fields
- Functions should not have too many parameters
- Methods should not have identical implementations
- Unused type declarations and private fields should be removed.
- Prefer passing large input values by read-only reference when supported and appropriate.
- Conditional operators should not be nested.
- Switch statements should not have too many case clauses.
- Loops should not have more than one break or goto statement.
- Loops with at most one iteration should be refactored.
- Choose `for` when initialization, condition, and iteration naturally belong together; otherwise prefer the loop form that makes control flow clear. The three expressions of a `for` statement should only be concerned with loop control.
- Switch statements should cover all cases.
- Values should not be uselessly incremented; empty statements should be removed.
- Remove redundant pairs of parentheses where they are not needed for clarity or to meet the precedence rule above.
- Array values should not be replaced unconditionally.
- Variables should not be accessed outside of their scope.
- Format strings should be used correctly and comply with ISO standards; width, alignment, and padding options should be used consistently.
- Mathematical constants should not be hardcoded.
- Values returned from string find-related methods should not be treated as boolean.
- Multiline blocks should be enclosed in braces. Conditionals should start on new lines, and conditionally executed single lines should be denoted by indentation.
- Multiple variables should not be declared on the same line.
- Avoid unnecessary copies, redundant casts, and unused declarations.

## Before finishing a change

- Review the changes against this standard, the C++ rules, and applicable repository and toolchain constraints. Apply each only within its stated scope.
- Check correctness, resource and object lifetimes, memory bounds, concurrency, portability, and preservation of required behavior and interfaces.
- Apply Uncrustify using the repository's `dev/uncrustify.cfg` to in-scope source and header files created or modified; do not reformat unrelated or excluded files.
- Check naming, documentation, variable scope, side effects, magic numbers, return paths, and applicable language-version and compiler settings, including warnings as errors.
- Build the affected code and run relevant tests when practical. Add or update tests when needed to verify changed behavior. Unit tests and fixtures do not have to follow these standards.
- Review the final diff for unintended or unrelated changes. Document exceptions as required by this standard.
- Report the checks performed, their results, and any relevant checks that could not be performed.
