import os
import subprocess
import sys

def ask_yes_no(question, default="y"):
    valid = {"y": True, "yes": True, "n": False, "no": False}
    if default is None:
        prompt = " [y/n] "
    elif default == "y":
        prompt = " [Y/n] "
    elif default == "n":
        prompt = " [y/N] "
    else:
        raise ValueError(f"Invalid default answer: '{default}'")
    
    while True:
        print(f"\n{question}")
        choice = input(prompt).lower().strip()
        
        if default is not None and choice == "":
            return valid[default]
        if choice in valid:
            return valid[choice]
        print("Please respond with [y] or [n].")

def main():
    cmake_options = [
        "-G", "Visual Studio 17 2022",
        "-DCMAKE_GENERATOR_TOOLSET=ClangCL"
    ]
    
    print("=" * 60)
    print("CMake Configuration Script")
    print("=" * 60)
    
    if ask_yes_no("Enable sanitizers?", default="n"):
        cmake_options.append('-DLLVM_ENABLE_PROJECTS="clang"')
        cmake_options.append('-DLLVM_ENABLE_RUNTIMES="compiler-rt"')
        
        if ask_yes_no("  Add AddressSanitizer?"):
            cmake_options.append("-DCRUDE_ADDRESS_SANITIZER=ON")
        
        if ask_yes_no("  Add MemorySanitizer?"):
            cmake_options.append("-DCRUDE_MEMORY_SANITIZER=ON")
        
        if ask_yes_no("  Add UndefinedBehaviorSanitizer?"):
            cmake_options.append("-DCRUDE_UNDEFINED_BEHAVIOR_SANITIZER=ON")
    
    # Ask about build type
    #print("\nSelect build type:")
    #print("1. Release (default)")
    #print("2. Debug")
    #print("3. RelWithDebInfo")
    #print("4. MinSizeRel")
    #
    #build_choices = {
    #    "1": "Release",
    #    "2": "Debug", 
    #    "3": "RelWithDebInfo",
    #    "4": "MinSizeRel"
    #}
    #
    #while True:
    #    choice = input("Enter choice [1]: ").strip() or "1"
    #    if choice in build_choices:
    #        cmake_options.append(f"-DCMAKE_BUILD_TYPE={build_choices[choice]}")
    #        break
    #    print("Invalid choice. Please enter 1-4.")
    
    # Ask for additional custom options
    print("\nEnter additional CMake options (space-separated, or press Enter to skip):")
    custom = input("Custom options: ").strip()
    if custom:
        cmake_options.extend(custom.split())
    
    # Show final command
    print("\n" + "=" * 60)
    print("Final CMake command:")
    print(f"cmake {' '.join(cmake_options)} ../")
    print("=" * 60)
    
    # Ask for confirmation
    if not ask_yes_no("\nRun CMake with these options?", default="y"):
        print("Cancelled.")
        return
    
    # Create build directory and run CMake
    os.makedirs("build", exist_ok=True)
    os.chdir("build")
    
    try:
        # Run CMake
        cmd = ["cmake"] + cmake_options + ["../"]
        print(f"\nExecuting: {' '.join(cmd)}")
        result = subprocess.run(cmd, check=True)
        
        if result.returncode == 0:
            print("\n✓ CMake completed successfully!")
        else:
            print(f"\n✗ CMake failed with code {result.returncode}")
            
    except subprocess.CalledProcessError as e:
        print(f"\n✗ CMake failed: {e}")
    except FileNotFoundError:
        print("\n✗ Error: 'cmake' command not found. Make sure CMake is in your PATH.")
    
    input("\nPress Enter to exit...")

if __name__ == "__main__":
    main()