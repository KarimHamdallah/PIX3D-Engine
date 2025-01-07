import os
import uuid
import argparse
import shutil
import xml.etree.ElementTree as ET

def update_solution_file(solution_path, project_name, project_path, project_guid):
    """
    Update the solution file to include the new project and set it as startup project
    """
    with open(solution_path, 'r') as f:
        solution_content = f.read()

    # Check if project is already in the solution
    if f'Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "{project_name}"' in solution_content:
        print(f"Project {project_name} already exists in the solution.")
        return

    # Prepare project reference
    project_reference = f'''
Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "{project_name}", "{os.path.relpath(project_path, os.path.dirname(solution_path))}", "{{{project_guid}}}"
EndProject
'''

    # Find where to insert the new project reference (before Global section)
    global_index = solution_content.find('Global')
    updated_solution = (
        solution_content[:global_index] + 
        project_reference + 
        solution_content[global_index:]
    )

    # Update ProjectConfigurationPlatforms section
    platforms_pattern = r'(GlobalSection\(ProjectConfigurationPlatforms\) = postSolution)(.*?)(EndGlobalSection)'
    platforms_match = re.search(platforms_pattern, updated_solution, re.DOTALL)
    
    if platforms_match:
        platforms_content = platforms_match.group(2)
        new_platforms_content = platforms_content + f'''
        {{{project_guid}}}.Debug|x64.ActiveCfg = Debug|x64
        {{{project_guid}}}.Debug|x64.Build.0 = Debug|x64
        {{{project_guid}}}.Release|x64.ActiveCfg = Release|x64
        {{{project_guid}}}.Release|x64.Build.0 = Release|x64
'''
        updated_solution = updated_solution.replace(
            platforms_match.group(0), 
            f'{platforms_match.group(1)}{new_platforms_content}{platforms_match.group(3)}'
        )

    # Set the new project as startup project
    solution_lines = updated_solution.split('\n')
    for i, line in enumerate(solution_lines):
        if line.strip().startswith('GlobalSection(SolutionProperties) = preSolution'):
            solution_lines.insert(i+1, f'\t\tSolutionGUID = {{{project_guid}}}')
            break

    # Write the updated solution file
    with open(solution_path, 'w') as f:
        f.write('\n'.join(solution_lines))

def get_project_guid(vcxproj_path):
    """
    Extract the project GUID from an existing .vcxproj file
    """
    try:
        tree = ET.parse(vcxproj_path)
        root = tree.getroot()
        ns = {'ns': 'http://schemas.microsoft.com/developer/msbuild/2003'}
        
        # Find the ProjectGuid element
        project_guid_elem = root.find(".//ns:ProjectGuid", namespaces=ns)
        
        if project_guid_elem is not None:
            # Remove curly braces and return uppercase GUID
            return project_guid_elem.text.strip('{}').upper()
        
    except Exception as e:
        print(f"Could not extract GUID from {vcxproj_path}: {e}")
        # Fallback to generating a new GUID if extraction fails
        return str(uuid.uuid4()).upper()

def create_visual_studio_project(project_name, engine_project_path, output_directory):
    """
    Create a new Visual Studio C++ project with references to an existing game engine project.
    """
    # Convert engine project path to absolute path
    engine_project_path = os.path.abspath(engine_project_path)
    
    # Determine solution path in the parent directory of the main solution
    solution_path = os.path.join(output_directory, f"{project_name}.sln")

    # Create project directory
    project_dir = os.path.join(output_directory, project_name)
    os.makedirs(project_dir, exist_ok=True)

    # Generate project GUID
    project_guid = str(uuid.uuid4()).upper()
    
    # Get engine project GUID
    engine_vcxproj_path = os.path.join(engine_project_path, "PIX3D.vcxproj")
    engine_project_guid = get_project_guid(engine_vcxproj_path)

    # Create solution file outside the project folder
    create_solution_file(solution_path, project_name, project_guid, engine_project_path, engine_project_guid)

    # Create project file
    project_dir = os.path.join(output_directory, project_name)
    project_path = os.path.join(project_dir, f"{project_name}.vcxproj")
    create_project_file(project_path, project_name, project_guid, engine_project_path, engine_project_guid, project_dir)

    # Create project filters file
    filters_path = os.path.join(project_dir, f"{project_name}.vcxproj.filters")
    create_project_filters_file(filters_path)

    # Create main source file
    source_dir = os.path.join(project_dir, "src")
    os.makedirs(source_dir, exist_ok=True)
    main_cpp_path = os.path.join(source_dir, "main.cpp")
    create_main_cpp(main_cpp_path, project_name)
    
    # Determine the location of the ImGui folder in the vendor directory
    _vendor_path = os.path.abspath(os.path.join(engine_project_path, '..', 'Vendor'))
    imgui_folder_path = os.path.join(_vendor_path, "imgui")

    # Determine the destination path in the application project
    application_imgui_path = os.path.join(project_dir, "imgui")

    # Create the destination folder if it doesn't exist
    os.makedirs(application_imgui_path, exist_ok=True)

    # Copy the ImGui folder and its contents
    shutil.copytree(imgui_folder_path, application_imgui_path, dirs_exist_ok=True)

    print(f"ImGui folder copied from {imgui_folder_path} to {application_imgui_path}")

    # Update the main solution file to include the new project
    update_solution_file(solution_path, project_name, project_path, project_guid)

    print(f"Project '{project_name}' created successfully in {project_dir}")
    print(f"Solution file created at: {solution_path}")

def create_solution_file(solution_path, project_name, project_guid, engine_project_path, engine_project_guid):
    """Create a Visual Studio solution file with engine project reference."""
    solution_content = f"""
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.0.0.0
Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "{project_name}", "{project_name}/{project_name}.vcxproj", "{{{project_guid}}}"
EndProject
Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "PIX3D", "{os.path.relpath(engine_project_path, os.path.dirname(solution_path))}/PIX3D.vcxproj", "{{{engine_project_guid}}}"
EndProject
Global
    GlobalSection(SolutionConfigurationPlatforms) = preSolution
        Debug|x64 = Debug|x64
        Release|x64 = Release|x64
    EndGlobalSection
    GlobalSection(ProjectConfigurationPlatforms) = postSolution
        {{{project_guid}}}.Debug|x64.ActiveCfg = Debug|x64
        {{{project_guid}}}.Debug|x64.Build.0 = Debug|x64
        {{{project_guid}}}.Release|x64.ActiveCfg = Release|x64
        {{{project_guid}}}.Release|x64.Build.0 = Release|x64
        {{{engine_project_guid}}}.Debug|x64.ActiveCfg = Debug|x64
        {{{engine_project_guid}}}.Debug|x64.Build.0 = Debug|x64
        {{{engine_project_guid}}}.Release|x64.ActiveCfg = Release|x64
        {{{engine_project_guid}}}.Release|x64.Build.0 = Release|x64
    EndGlobalSection
    GlobalSection(ProjectDependencies) = postSolution
        {{{project_guid}}} = {{{engine_project_guid}}}
    EndGlobalSection
    GlobalSection(SolutionProperties) = preSolution
        HideSolutionNode = FALSE
    EndGlobalSection
EndGlobal
"""

    with open(solution_path, 'w') as f:
        f.write(solution_content)

def create_project_file(project_path, project_name, project_guid, engine_project_path, engine_project_guid, proj_dir):
    """Create a Visual Studio project file with engine reference."""
    # Calculate vendor and library paths relative to engine project
    vendor_path = os.path.abspath(os.path.join(engine_project_path, '..', 'Vendor'))
    glfw_lib_path = os.path.abspath(os.path.join(vendor_path, 'glfw', 'lib'))
    assimp_lib_path = os.path.abspath(os.path.join(vendor_path, 'assimp', 'bin', 'windows', 'Release'))    
    assimp_include_path = os.path.abspath(os.path.join(vendor_path, 'assimp', 'include'))
    

    project_content = f"""<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCProjectVersion>16.0</VCProjectVersion>
    <Keyword>Win32Proj</Keyword>
    <ProjectGuid>{{{project_guid}}}</ProjectGuid>
    <RootNamespace>{project_name}</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
    <IncludePath>{engine_project_path}\PIX3D;{vendor_path}\glm;{vendor_path}\glfw\include;{proj_dir}\imgui;{assimp_include_path};$(IncludePath)</IncludePath>
    <LibraryPath>{glfw_lib_path};{assimp_lib_path};$(LibraryPath)</LibraryPath>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
    <IncludePath>{engine_project_path}\PIX3D;{vendor_path}\glm;{vendor_path}\glfw\include;{proj_dir}\imgui;{assimp_include_path};$(IncludePath)</IncludePath>
    <LibraryPath>{glfw_lib_path};{assimp_lib_path};$(LibraryPath)</LibraryPath>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>PIX_DEBUG;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <AdditionalLibraryDirectories>{glfw_lib_path};{assimp_lib_path};</AdditionalLibraryDirectories>
      <AdditionalDependencies>glfw3.lib;opengl32.lib;assimp-vc143-mt.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>PIX_RELEASE;NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp20</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <AdditionalLibraryDirectories>{glfw_lib_path};{assimp_lib_path};</AdditionalLibraryDirectories>
      <AdditionalDependencies>glfw3.lib;opengl32.lib;assimp-vc143.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="src\main.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ProjectReference Include="../{os.path.basename(engine_project_path)}/PIX3D.vcxproj">
      <Project>{{{engine_project_guid}}}</Project>
    </ProjectReference>
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>
"""
    with open(project_path, 'w') as f:
        f.write(project_content)

# The rest of the functions remain the same as in the previous version
def create_project_filters_file(filters_path):
    """Create a basic project filters file."""
    filters_content = """<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup>
    <Filter Include="Source Files">
      <UniqueIdentifier>{4FC737F1-C7A5-4376-A066-2A32D752A2FF}</UniqueIdentifier>
      <Extensions>cpp;c;cc;cxx;c++;cppm;ixx;def;odl;idl;hpj;bat;asm;asmx</Extensions>
    </Filter>
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="src\main.cpp">
      <Filter>Source Files</Filter>
    </ClCompile>
  </ItemGroup>
</Project>
"""
    with open(filters_path, 'w') as f:
        f.write(filters_content)

def create_main_cpp(main_cpp_path, project_name):
    """Create a basic main.cpp file with the project name embedded in specs.Title."""
    main_content = """
/*
* Copyright (c) 2024 Karim Hamdallah
*/

#include <PIX3D.h>

int main()
{
    // Init Engine
    {
        PIX3D::EngineSpecs EngineSpecs;
        EngineSpecs.API = PIX3D::GraphicsAPI::OPENGL;

        PIX3D::Engine::Init(EngineSpecs);
    }

    // Run Application
    PIX3D::ApplicationSpecs specs;
    specs.Width = 800;
    specs.Height = 600;
    specs.Title = "{project_name}";  // Embedded project name here
    
    PIX3D::Engine::CreateApplication<PIX3D::Application>(specs);

    // Destroy Engine
    PIX3D::Engine::Destroy();

    return 0;
}
"""
    # Replace `{project_name}` with the actual value of the project_name variable
    main_content = main_content.replace("{project_name}", project_name)

    with open(main_cpp_path, 'w') as f:
        f.write(main_content)

def main():
    parser = argparse.ArgumentParser(description='Create a new Visual Studio C++ project with game engine reference')
    parser.add_argument('project_name', help='Name of the new project')
    parser.add_argument('engine_project_path', help='Path to the game engine project')
    parser.add_argument('--output', default=None, help='Output directory for the new project (default: current directory)')

    args = parser.parse_args()

    # Use current directory if no output specified
    output_dir = args.output if args.output else os.getcwd()

    create_visual_studio_project(
        args.project_name, 
        args.engine_project_path, 
        output_dir
    )

if __name__ == "__main__":
    main()