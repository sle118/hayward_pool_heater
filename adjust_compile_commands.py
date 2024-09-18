import os
import subprocess
import json
import logging
import argparse
import yaml
import configparser
import glob
PLATFORMIO_PACKAGES_DIR = os.path.expanduser('~/.platformio/packages/')
ignored_build_flags = ['-fno-tree-switch-conversion','-fstrict-volatile-bitfields','-mlongcalls']
toolchain_paths = []



def find_standard_includes(toolchain_paths):
    include_dirs = []

    for toolchain_path in toolchain_paths:
        # Use glob to find any directory under the toolchain path that looks like an include directory
        toolchain_base = os.path.dirname(toolchain_path) if toolchain_path.endswith('bin') else toolchain_path
        include_patterns = [
            os.path.join(toolchain_base, '**/include'),  # Matches all include directories
            os.path.join(toolchain_base, '**/include/c++/*'),  # Matches C++ version directories
            os.path.join(toolchain_base, '**/lib/gcc/*/*/include')  # Matches GCC-specific include directories
        ]

        # Add directories that match the glob patterns
        for pattern in include_patterns:
            matched_dirs = glob.glob(pattern, recursive=True)
            for dir in matched_dirs:
                if os.path.isdir(dir):  # Only include actual directories
                    include_dirs.append(os.path.abspath(dir))
                    logging.info(f"Found include directory: {os.path.abspath(dir)}")

    return include_dirs


def build_source_file_map(output_folder):
    """Build a dictionary of source file names mapped to their full paths in the output folder."""
    source_file_map = {}

    # Recursively glob for all files in the output folder
    for filepath in glob.glob(os.path.join(output_folder, '**', '*.*'), recursive=True):
        filename = os.path.basename(filepath)
        source_file_map[filename] = filepath

    return source_file_map
# Initialize logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
def adjust_source_file_path(file_path, source_file_map):
    """
    Adjust source file paths that are in the build folder to point to the actual source files
    in the script folder (project folder) using the source file map.
    """
    # Extract the filename from the file path
    filename = os.path.basename(file_path)

    # Check if this file exists in the source file map
    if filename in source_file_map:
        # Return the path to the corresponding source file in the project folder
        return os.path.abspath(source_file_map[filename])

    # If no match is found, return the original file path
    return file_path



def find_tools_paths(compile_commands) -> list[str]: 
    """Build a list of compiler paths from the compile_commands."""
    tools_paths = []
    
    missing_paths = []


    for entry in compile_commands:

        command = entry.get('command', '')
        parts = command.split()

        if parts[0] not in tools_paths and os.path.exists(parts[0]):
            tools_paths.append(parts[0])
            # also add to bin_paths if not already there
            if parts[0] not in toolchain_paths:
                # append the directory of the tool to bin_paths
                toolchain_paths.append(os.path.dirname(parts[0]))
        else:
            # append to missing_paths if not already there and not in tools_paths
            if parts[0] not in missing_paths and parts[0] not in tools_paths:
                missing_paths.append(parts[0])
            
        
    # build a list of missing_paths for which the string was not found in tools_paths
    missing_paths = [path for path in missing_paths if not any(path in tools_path for tools_path in tools_paths)]
    


    # for each remaining object in missing_paths, see if the tool exists in one of the bin_paths
    # if so, add it to tools_paths, if not, log a warning
    for path in missing_paths:
        found = False
        for bin_path in toolchain_paths:
            if os.path.exists(os.path.join(bin_path, path)):
                tools_paths.append(os.path.join(bin_path, path))
                found = True
                break

        if not found:
            logging.warning(f"Tool '{path}' not found in any of the toolchain_paths: {toolchain_paths}")

        





            
    return tools_paths

def check_and_generate_compile_commands(build_folder, refresh=False):
    """Check if compile_commands.json exists and generate it using pio if not or if refresh is set."""
    compile_commands_path = os.path.join(build_folder, 'compile_commands.json')

    if refresh and os.path.exists(compile_commands_path):
        logging.info(f"Removing existing {compile_commands_path} as --refresh option was specified.")
        os.remove(compile_commands_path)

    if not os.path.exists(compile_commands_path):
        logging.warning(f"{compile_commands_path} not found. Generating compile_commands.json.")

        # Run the pio command to generate compile_commands.json
        try:
            result = subprocess.run(['pio', 'run', '-t', 'compiledb', '-d', build_folder],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

            if result.returncode == 0:
                logging.info("Successfully generated compile_commands.json.")
            else:
                logging.error(f"Failed to generate compile_commands.json: {result.stderr}")
                raise RuntimeError("PlatformIO command failed.")
        except FileNotFoundError:
            logging.error("PlatformIO (pio) is not installed or not in the system PATH.")
            raise
    else:
        logging.info(f"{compile_commands_path} already exists.")

def read_compile_commands(build_folder):
    """Read compile_commands.json from the build folder."""
    compile_commands_path = os.path.join(build_folder, 'compile_commands.json')
    with open(compile_commands_path, 'r') as f:
        return json.load(f)

def resolve_path(build_folder, relative_path):
    """Resolve paths that aren't found by prepending the build folder path."""
    potential_path = os.path.join(build_folder, relative_path)
    if os.path.exists(potential_path):
        return os.path.realpath(potential_path)
    return None

def update_clangd_file(include_paths, output_folder, output_compile_commands_path,platformio_defines=None):
    """Update the .clangd file with the new include paths and platformio definitions."""
    clangd_path = os.path.join(output_folder, '.clangd')

    # Check if pyyaml is available
    try:
        import yaml
    except ImportError:
        logging.error("PyYAML is not installed. Install it by running 'pip install pyyaml'.")
        return

    # Load existing .clangd file if it exists
    if os.path.exists(clangd_path):
        with open(clangd_path, 'r') as f:
            clangd_config = yaml.safe_load(f)
    else:
        clangd_config = {}

    # Update the CompileFlags section with new include paths
    if 'CompileFlags' not in clangd_config:
        clangd_config['CompileFlags'] = {}

    # Ensure Add exists
    if 'Add' not in clangd_config['CompileFlags']:
        clangd_config['CompileFlags']['Add'] = []

    # remove any entry from Add that don't start with - as they are invalid
    clangd_config['CompileFlags']['Add'] = [flag for flag in clangd_config['CompileFlags']['Add'] if flag.startswith('-')]

    standard_include_dirs = find_standard_includes(toolchain_paths)


    for include_dir in standard_include_dirs:
        include_flag = f"-I{include_dir}"
        if include_flag not in clangd_config['CompileFlags']['Add']:
            clangd_config['CompileFlags']['Add'].append(include_flag)

    clangd_config['CompileFlags']['Add'] = list(set(clangd_config['CompileFlags']['Add']))
    clangd_config['CompileFlags']['Add'].sort()


    # Add unique flags to the CompileFlags.Add section
    existing_flags = set(clangd_config['CompileFlags']['Add'])
    # add  project directory to include paths
    include_paths.append(os.path.dirname(output_compile_commands_path))
    # Add include paths
    for path in include_paths:
        include_flag = f"-I{path}"
        if include_flag not in existing_flags:
            clangd_config['CompileFlags']['Add'].append(include_flag)
            existing_flags.add(include_flag)
    # Add platformio defines
    if platformio_defines:
        for define in platformio_defines:
            define_flag = f"-D{define}"
            if define_flag not in existing_flags:
                clangd_config['CompileFlags']['Add'].append(define_flag)
                existing_flags.add(define_flag)


    # add CompilationDatabase entry in CompileFlags, using the containing directory
    # of output_compile_commands_path
    clangd_config['CompileFlags']['CompilationDatabase'] = os.path.dirname(output_compile_commands_path)
    

    # Save updated .clangd file
    with open(clangd_path, 'w') as f:
        yaml.dump(clangd_config, f)
    logging.info(f"Updated .clangd file with include paths in {clangd_path}")

def extract_defines_from_platformio(build_folder):
    """Extract platformio defines and flags from platformio.ini."""
    platformio_ini_path = os.path.join(build_folder, 'platformio.ini')
    if not os.path.exists(platformio_ini_path):
        logging.error(f"platformio.ini not found in {build_folder}")
        return None

    # Use configparser to parse the platformio.ini
    config = configparser.ConfigParser()
    config.read(platformio_ini_path)

    defines = []
    
    for section in config.sections():
        if section.startswith("env:"):
            if 'build_flags' in config[section]:
                flags = config[section]['build_flags']
                for flag in flags.split():
                    if flag.startswith("-D"):
                        defines.append(flag[2:])
            if 'platform_packages' in config[section]:
                packages = config[section]['platform_packages'].splitlines()
                for package in packages:
                    if len(package)== 0 or  not 'toolchain'  in package:
                        continue
                    package_name = package.split('@')[0].strip()
                    # Map to actual folder in ~/.platformio/packages
                    package_dir = os.path.join(PLATFORMIO_PACKAGES_DIR, package_name.split('/')[-1])
                    if os.path.exists(package_dir):
                        toolchain_paths.append(package_dir)
                

    logging.info(f"Extracted {len(defines)} defines from platformio.ini.")
    return defines
def process_compile_commands(compile_commands, output_folder, build_folder, platformio_defines=None):
    """Process compile_commands.json, adjust paths, collect include paths, and write modified compile_commands.json to output folder."""
    include_paths = set()
    modified_compile_commands = []
    
    # Build a map of source files from the output folder (project folder)
    source_file_map = build_source_file_map(output_folder)

    # Find tools paths
    tools_list = find_tools_paths(compile_commands)
    # log the compiler paths/tools that were found, one per line,
    # using join
    logging.info(f"Found compiler paths: {', '.join(tools_list)}")


    

    for entry in compile_commands:
        # Collect all include paths from each command
        command = entry.get('command', '')
        new_command_parts = []

        # Split the command into parts
        command_parts = command.split()

        # Loop through each part of the command
        for idx, part in enumerate(command_parts):

            # with the first part being the command executable, check if the 
            # executable name is found in compiler and if so, append the full path 
            # otherwise append the part itself.
            if idx==0:
                # check if part matches a substring in the tools list
                if any(part in tools for tools in tools_list):
                    # append the matching tool from tools_list  
                    new_command_parts.append(next(tool for tool in tools_list if part in tool))
                else:
                    new_command_parts.append(part)

            if part.startswith('-I'):
                include_path = part[2:]  # Remove the '-I' prefix

                # Check if the path exists, if not try resolving it using the build folder
                if not os.path.exists(include_path):
                    resolved_path = resolve_path(build_folder, include_path)
                    if resolved_path:
                        include_paths.add(resolved_path)
                        new_command_parts.append(f"-I{resolved_path}")
                        logging.debug(f"Resolved missing path: {resolved_path}")
                    else:
                        new_command_parts.append(part)
                        logging.warning(f"Could not resolve missing path: {include_path}")
                else:
                    realpath = os.path.realpath(include_path)
                    include_paths.add(realpath)
                    new_command_parts.append(f"-I{realpath}")
            else:
                # append if not in ignored_build_flags
                if part not in ignored_build_flags:
                    new_command_parts.append(part)
                

        # Reconstruct the command
        new_command = ' '.join(new_command_parts)
        entry['command'] = new_command

        # Adjust 'file' path using the source file map
        entry['file'] = adjust_source_file_path(os.path.abspath(entry['file']), source_file_map)

        # Adjust 'directory' to an absolute path
        entry['directory'] = os.path.abspath(entry['directory'])

        modified_compile_commands.append(entry)

    # remove the path to the build folder from include_paths
    include_paths.discard(os.path.realpath(os.path.join(build_folder,'src')))

    

    # Write the modified compile_commands.json to the output folder
    output_compile_commands_path = os.path.join(output_folder, 'compile_commands.json')
    with open(output_compile_commands_path, 'w') as f:
        json.dump(modified_compile_commands, f, indent=2)
    logging.info(f"Modified compile_commands.json written to {output_compile_commands_path}")

    # Convert set to list and remove any invalid paths
    # and keep distinct paths

    valid_include_paths = list(set([path for path in include_paths if os.path.isdir(path)]))
    
    # Log any invalid paths
    invalid_paths = [path for path in include_paths if not os.path.isdir(path)]
    if invalid_paths:
        logging.warning("The following highest-level include paths were not found:")
        for path in invalid_paths:
            logging.warning(f"- {path}")

    # Update the .clangd file with valid include paths and platformio defines
    update_clangd_file(valid_include_paths, output_folder, output_compile_commands_path,platformio_defines)


def main():
    parser = argparse.ArgumentParser(description="Process compile_commands.json and update .clangd.")
    parser.add_argument('--build-folder', required=True, help="Path to the build folder.")
    parser.add_argument('--out-folder', default= os.path.dirname(os.path.abspath(__file__)), help="Output folder for the modified compile_commands.json and .clangd.")
    parser.add_argument('--refresh', action='store_true', help="Force regeneration of compile_commands.json.")

    args = parser.parse_args()
    os.chdir(args.build_folder)

    # Step 1: Check if compile_commands.json exists, and generate it if not (or refresh if requested)
    check_and_generate_compile_commands(args.build_folder, refresh=args.refresh)

    # Step 2: Extract defines and flags from platformio.ini
    platformio_defines = extract_defines_from_platformio(args.build_folder)

    # Step 3: Read the compile_commands.json
    compile_commands = read_compile_commands(args.build_folder)

    # Step 4: Process compile_commands.json, adjust paths, collect include paths, and write modified compile_commands.json to output folder
    process_compile_commands(compile_commands, args.out_folder, args.build_folder, platformio_defines)

if __name__ == "__main__":
    main()