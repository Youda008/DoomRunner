#!/usr/bin/python3

# Recursively inspects the dependencies of the selected executable and prints the unique library paths, each on its own line.

import sys
import os
import argparse
import subprocess


def printerr(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    
    
def parent_dir(path):
    return os.path.join(path, os.pardir)
    

def get_direct_deps(bin_path):

    try:
        result = subprocess.run(['otool', '-L', bin_path], capture_output=True, text=True, check=True)
        lines = result.stdout.splitlines()
        
        deps = []
        
        # first line is the file name, subsequent lines are deps
        for line in lines[1:]:
            # extract the path (the first string on the line)
            parts = line.strip().split()
            if parts and len(parts) > 0:
                deps.append(parts[0])
            else:
                printerr(f"Warning: unparsable line: {line}")
                
        return deps
    except subprocess.CalledProcessError as e:
        printerr(f"Error: otool failed: {e}")
        return []


def dependency_path(bin_path):
    if ".framework/" in bin_path:
        # If the dependency is a framework, use the framework dir path rather than the binary path
        # Example: Contents/Frameworks/QtCore.framework/Versions/A/QtCore
        # Result:  Contents/Frameworks/QtCore.framework
        return parent_dir(parent_dir(parent_dir(bin_path)))
    else:
        # Example: Contents/Frameworks/libssl.3.dylib
        return bin_path


def get_deps_recursively(exe_path, bin_path, visited_deps, excluded_prefixes):
  
    for dep_path in get_direct_deps(bin_path):
        
        orig_dep_path = dep_path
        
        # resolve otool placeholders
        if dep_path.startswith("@loader_path"):
            dep_path = dep_path.replace("@loader_path", parent_dir(bin_path))
        if dep_path.startswith("@rpath"):
            dep_path = dep_path.replace("@rpath", parent_dir(dependency_path(bin_path)))
        if dep_path.startswith("@executable_path"):
            dep_path = dep_path.replace("@executable_path", parent_dir(exe_path))
                    
        # resolve relative parts and symlinks
        dep_real_path = os.path.realpath(dep_path)         
        
        # check the exclusions
        if any(dep_real_path.startswith(p) for p in excluded_prefixes):
            continue
        
        # don't traverse already visited dependencies
        if dep_real_path in visited_deps:
            continue
            
        visited_deps.add(dep_real_path)
        
        if not os.path.exists(dep_real_path):
            printerr(f"Warning: Dependency '{dep_real_path}' does not exist")
            if dep_real_path != orig_dep_path:
                printerr(f"         otool path: {orig_dep_path}")
            continue
    
        get_deps_recursively(exe_path, dep_real_path, visited_deps, excluded_prefixes)


def main():

    parser = argparse.ArgumentParser()
    # positional arguments
    parser.add_argument("binary_path", help="a binary to be inspected - can be an executable or a library")
    # options that take a value
    parser.add_argument("-e", "--executable-path", help="original executable at which the dependency inspection started, required to resolve @executable_path placehoder")
    args = parser.parse_args()
    if not args or not args.binary_path:
        parser.print_help()
        sys.exit(1)

    if not os.path.exists(args.binary_path):
        printerr(f"Error: '{args.binary_path}' does not exist")
        sys.exit(2)
    bin_real_path = os.path.realpath(args.binary_path)

    if args.executable_path:
        if not os.path.exists(args.executable_path):
            printerr(f"Error: '{args.executable_path}' does not exist")
            sys.exit(2)
        exe_real_path = os.path.realpath(args.executable_path)
    else:
        exe_real_path = bin_real_path
    
    unique_deps = set()
    excluded = [
        '/System/Library',
        '/usr/lib'
    ]

    get_deps_recursively(exe_real_path, bin_real_path, unique_deps, excluded)

    # remove the original executable from the output set
    if bin_real_path in unique_deps:
        unique_deps.remove(bin_real_path)

    # print results
    for dep_path in sorted(unique_deps):
        # the result contains invalid ones to prevent printing duplicated warning messages
        if os.path.exists(dep_path):
            print(dep_path)


if __name__ == "__main__":
    main()
