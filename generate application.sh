#!/bin/bash
# Prompt the user for the project name
read -p "Enter the desired project name: " project_name
# Run the Python script with the user-provided project name
python ApplicationCreatorScript.py "$project_name" "PIX3D"

# Wait for user input before closing
echo "Press Enter to exit..."
read