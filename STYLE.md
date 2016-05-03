# Naming

  - Class/Struct/Union Names: lower case, underscore style (i.e. `my_class`)
  - Function Names: lower case, underscore style (i.e. `my_function`)
  - Scope Variables: lower case, underscore style (i.e. `my_var`)
  - Template Parameters: camel case (i.e. `MyTemplateVar`)
  - Member Variables: lower case, underscore, "_" postfix (i.e. `my_member_var_`)
  - Constants (i.e. in enums): upper case, underscore (i.e. `enum class directions { ONE_TO_MANY, MANY_TO_ONE };` )

Basically, this leads to a consistent code impression when using the STL or Boost libraries.

Do not use very long variable and class names.


# Formatting

Use clang-format (the .clang-format file is located in this repository).

Don't leave out brackets:

    // Wrong.
    if (exit) return;

    // Correct.
    if (exit) {
      return;
    }


Minimize use of vertical whitespace:
Don't put more than one or two blank lines between functions, resist starting functions with a blank line, don't end functions with a blank line, and be discriminating with your use of blank lines inside functions. The basic principle is: The more code that fits on one screen, the easier it is to follow and understand the control flow of the program. Of course, readability can suffer from code being too dense as well as too spread out, so use your judgement. But in general, minimize use of vertical whitespace.

(Source: https://google-styleguide.googlecode.com/svn/trunk/cppguide.html#Vertical_Whitespace)


# Comments

  - Do not commit commented out code (this also applies to `#ifdef 0` or `if (false) {...}`).
  - Do not write comments that do not explain anything like:

        /**
         * @return  returns the index
         */
        int index() { return index_; }

  - Do not explain language/library features in your comments.
  - Do not repeat what the code does (the reader can read the code, too) but describe the concepts and reasons behind it, **if** it is not obvious.
  - Do not describe algorithms like Dijkstra, etc. (this can be better explained in Thesis documents using pseudo code, etc. or looked up on the Internet).
  - If you comment code: would it be possible to skip the comment if you split your method, give your method(s)/classes(s) a more meaningful name, give your variables more meaningful names, etc.?
  - If you comment code: try to use full sentences and correct English


# Files

File extensions:

  - C (implementation) files: ".cc"
  - H (header) files: ".h"

Put header files in a unique folder structure (i.e. /include/motis/...).


# Namespaces

Use your own unique namespace to prevent collisions (i.e. motis::search).
Mark the namespace end bracket with a comment:

    namespace test {
    /* .. your implementation (many lines) */
    }  // namespace test

Otherwise, one may wonder what this single closing bracket is good for.
Do not indent namespaces.

  - Do **not** (at least not globally) import the `std::` namespace using `using namespace std;`.
  - Do **not** import any namespace or elements of namespace in header files.
  - If your code becomes unreadable due to long namespace names, rename them in the `.cc` file or (if appropriate) in a single function scope: `namespace bg = boost:geometry;`. It may also be sufficient to import single elements of a namespace (i.e. `using std::get;`).


# Includes

Ordering/Groups (separated by empty lines):

  - System C includes: #include <cstdlib>
  - System includes: #include <iostream>
  - Library includes: #include "boost/..."
  - Own includes: #include "motis/..."
  - Use forward-declarations instead of includes where possible.
    This improves build times and hides details.


# Programming

(Main Source: https://google-styleguide.googlecode.com/svn/trunk/cppguide.html)

  - Prefer references over pointers (i.e. for function parameters):
    - References do not have a "null" state (-> no checks required)
    - References imply that there is no memory management (new/delete) required
  - Prevent unnecessary copies by using references (or pointers if you cannot avoid them)
  - Const Correctness: Always use "const" where possible (function parameters, member functions, etc.)
  - Use C++11 features to improve readability: range-based for loops, auto, lambdas, braced initializer lists, nullptr instead of NULL, etc.
  - Use scoped stack variables where possible, avoid heap allocations.
  - No "new" and "delete" (there are exceptions - but use this as a basic rule): use smart pointers, vectors, etc. instead (RAII);
    Prefer automatic memory management because doing it manually is very error-prone.
  - Handle resources (files, connections, etc.) using RAII (-> no leak even in case of exceptions)
  - Avoid RTTI
  - Cast using static_cast (always if possible), const_cast, reinterpret_cast
  - Increment using prefix: ++i (otherwise, unnecessary temporaries are created)
  - Avoid global state (no global static variables, singletons, etc.)
  - Do not write simple setters and getters, just use public member variables
  - Try to avoid "output parameters" (better: input=parameter, output=return value)
  - Make sure the project does not compile too long (use forward declarations, pimpl idiom if necessary)
  - Make full use of the Standard Library to simplify your own code: I.e. use STL algorithms instead of writing your own.


# Software Design

Follow basic software engineering principles.

  - KISS, YAGNI:
     - write simple, readable code for required features (minimal solution)
     - do not solve problems that might occur in future
     - do not overuse design patterns where a simple solution is sufficient
  - "high cohesion, low coupling", SOLID principles, etc.
  - Use abstractions to hide details (abstract your business logic from your communication layer, persistence layer, view layer, etc.)
  - watch out for common code smells (http://en.wikipedia.org/wiki/Code_smell, http://de.slideshare.net/gvespucci/design-smells)
  - write tests if appropriate (possibly test-driven)


# Project Architecture

  - Think twice before introducing completely new library dependencies:
    - Do I really need this library or can I just reimplement a small subset
      of its functionality and therefore have a simpler solution?
    - Who maintains this library? How mature is it? Is it in production use somewhere?
    - Can you read and understand the code (i.e. to extend it or fix bugs)?
    - Does it have tests, examples, documentation, etc.?
  - Before introducing external dependencies like a MySQL server or a message queue (i.e. RabbitMQ): would a simple embedded database or a direct HTTP connection be sufficient?
  - Make it CMake compatible -> "cmake . && make" should build the complete project (including all libraries) for every platform (Mac OS, Linux: clang, gcc; Windows: MSVS).
    Even for non-host platforms like ARM/MIPS (crosstool-ng) or other operating systems like Windows (MinGW) using cross-toolchains.
  - Use the Git submodule feature and the CMake `add_subdirectory()` function to include the libraries CMakeLists.txt file.

(since Boost is often reference for the C++ standard, it plays a special role here)