import os
import re

def process_file(filename, processed_files=None):
    """
    Process an OpenSCAD file, replacing include statements with file contents
    
    Args:
        filename (str): The name of the file to process
        processed_files (set): Set of files already processed to avoid circular includes
    
    Returns:
        str: The processed file content
    """
    if processed_files is None:
        processed_files = set()
    
    # Check if file exists
    if not os.path.isfile(filename):
        print(f"Warning: File {filename} not found")
        return ""
    
    # Avoid circular includes
    if filename in processed_files:
        print(f"Warning: Circular include detected for {filename}, skipping")
        return ""
    
    processed_files.add(filename)
    output_lines = []
    
    with open(filename, 'r') as file:
        for line in file:
            # Check for include statements
            include_match = re.match(r'^\s*include\s*<\s*([^>]+)\s*>', line)
            if include_match:
                included_file = include_match.group(1)
                print(f"Processing include: {included_file}")
                
                # Recursively process the included file
                included_content = process_file(included_file, processed_files)
                output_lines.append(f"// Begin included from: {included_file}\n")
                output_lines.append(included_content)
                output_lines.append(f"// End included from: {included_file}\n")
            else:
                output_lines.append(line)
    
    return "".join(output_lines)

def main():
    input_file = "Main.scad"
    output_file = "MainParsed.scad"
    
    # Check if main file exists
    if not os.path.isfile(input_file):
        print(f"Error: {input_file} not found in the current directory")
        return
    
    print(f"Processing {input_file}...")
    processed_content = process_file(input_file)
    
    # Write the parsed output
    with open(output_file, 'w') as out_file:
        out_file.write(processed_content)
    
    print(f"Successfully created {output_file}")

if __name__ == "__main__":
    main()