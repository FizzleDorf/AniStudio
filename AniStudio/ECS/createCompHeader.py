import os

def generate_header_includes(directory, output_file):
    with open(output_file, "w") as out_file:
        out_file.write("#pragma once\n\n")
        for root, dirs, files in os.walk(directory):
            for file in files:
                if file.endswith(".h") or file.endswith(".hpp"):
                    relative_path = os.path.join(root, file).replace("\\", "/")  # Ensure forward slashes for includes
                    out_file.write(f'#include "{relative_path}"\n')

# Generate includes for components
components_dir = "components"
components_output_file = "components/components.h"
generate_header_includes(components_dir, components_output_file)

# Generate includes for systems
systems_dir = "systems"
systems_output_file = "systems/systems.h"
generate_header_includes(systems_dir, systems_output_file)
