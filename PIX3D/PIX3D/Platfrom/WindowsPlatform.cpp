#include "WindowsPlatform.h"
#include <Engine/Engine.hpp>
#include <Platfrom/GL/GLCommands.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Utils/stb_write.h>

#include <Windows.h>
#include <shlobj.h>
#include <commdlg.h>

#pragma comment(lib, "Shell32.lib")

#define GLFW_INCLUDE_VULKAN

namespace
{
    bool FirstMouse = true;

    void WindowResizeCallBack(GLFWwindow* window, int width, int height)
    {
        PIX3D::Engine::SetWindowWidth(width);
        PIX3D::Engine::SetWindowHeight(height);

        auto App = PIX3D::Engine::GetApplication();
        App->OnResize(width, height);

        PIX3D::GL::GLCommands::SetViewPort(width, height);
        PIX3D::GL::GLRenderer::Resize(width, height);

        auto func = PIX3D::Engine::GetWindowSizeCallBackFunc();
        if(func)
            func(width, height);
    }

    void KeyboardCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
    { 
        auto App = PIX3D::Engine::GetApplication();
        App->OnKeyPressed(key);

        auto func = PIX3D::Engine::GetKeyboardCallBackFuncPtr();
        if(func)
            func((PIX3D::KeyCode)key);
    }

    void CursorPositionCallBack(GLFWwindow* window, double xpos, double ypos)
    {
        if (FirstMouse)
        {
            PIX3D::Input::SetMousePosition(glm::vec2(xpos, ypos));
            FirstMouse = false;
        }

        auto lastmouse = PIX3D::Input::GetMousePosition();

        float xoffset = xpos - lastmouse.x;
        float yoffset = lastmouse.y - ypos;

        PIX3D::Input::SetMouseOffset(glm::vec2(xoffset, yoffset));
        PIX3D::Input::SetMousePosition(glm::vec2(xpos, ypos));
    }

    void MouseScrollCallBack(GLFWwindow* window, double xoffset, double yoffset)
    {
        PIX3D::Input::SetMouseScroll(glm::vec2(xoffset, yoffset));
    }



    std::vector<uint8_t> FlipTextureVertically(const std::vector<uint8_t>& pixels, int width, int height, int bpp)
    {
        // Create a temporary buffer to store flipped data
        std::vector<uint8_t> flipped(pixels.size());

        // Calculate the stride (number of bytes per row)
        int stride = width * bpp;

        // Iterate over each row and flip it
        for (int y = 0; y < height; ++y) {
            // Source row index
            int srcRow = y * stride;

            // Destination row index (flipped vertically)
            int dstRow = (height - 1 - y) * stride;

            // Copy the entire row
            std::copy(pixels.begin() + srcRow, pixels.begin() + srcRow + stride, flipped.begin() + dstRow);
        }

        return flipped;
    }
}

namespace PIX3D
{
    WindowsPlatformLayer::~WindowsPlatformLayer()
    {
        glfwDestroyWindow((GLFWwindow*)m_NativeWindowHandel);
        glfwTerminate();
    }

    void WindowsPlatformLayer::CreatWindow(uint32_t width, uint32_t height, const char* title, bool resizable)
    {
        // init glfw
        PIX_ASSERT_MSG(glfwInit(), "Failed To Initialize GLFW!");

        // check for graphics api
        auto EngineSpecs = Engine::GetEngineSpecs();

        if (EngineSpecs.API == GraphicsAPI::OPENGL) // opengl
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_RESIZABLE, resizable);
            glfwWindowHint(GLFW_SAMPLES, 8);
        }
        if (EngineSpecs.API == GraphicsAPI::VULKAN) // vulkan
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, resizable);
        }

        // create window
        m_NativeWindowHandel = glfwCreateWindow(width, height, title, NULL, NULL);

        // assert window creation
        PIX_ASSERT_MSG(m_NativeWindowHandel, "Failed To Create GLFW Window!");

        // set callbacks
        glfwSetWindowSizeCallback((GLFWwindow*)m_NativeWindowHandel, WindowResizeCallBack);
        glfwSetKeyCallback((GLFWwindow*)m_NativeWindowHandel, KeyboardCallBack);
        glfwSetCursorPosCallback((GLFWwindow*)m_NativeWindowHandel, CursorPositionCallBack);
        glfwSetScrollCallback((GLFWwindow*)m_NativeWindowHandel, MouseScrollCallBack);
    }

    void WindowsPlatformLayer::PollEvents()
    {
        glfwPollEvents();
    }

    void* WindowsPlatformLayer::GetNativeWindowHandel()
    {
        return m_NativeWindowHandel;
    }
    std::pair<uint32_t, uint32_t> WindowsPlatformLayer::GetWindowSize()
    {
        int w, h;
        glfwGetWindowSize((GLFWwindow*)m_NativeWindowHandel, &w, &h);

        return { w, h };
    }

    bool WindowsPlatformLayer::IsKeyPressed(int keycode)
    {
        bool pressed = glfwGetKey((GLFWwindow*)m_NativeWindowHandel, keycode) == GLFW_PRESS;
        return pressed;
    }

    bool WindowsPlatformLayer::IsKeyReleased(int keycode)
    {
        bool released = glfwGetKey((GLFWwindow*)m_NativeWindowHandel, keycode) == GLFW_RELEASE;
        return released;
    }

    bool WindowsPlatformLayer::IsMouseButtonPressed(int mousebutton)
    {
        bool pressed = glfwGetMouseButton((GLFWwindow*)m_NativeWindowHandel, mousebutton) == GLFW_PRESS;
        return pressed;
    }

    bool WindowsPlatformLayer::IsMouseButtonReleased(int mousebutton)
    {
        bool released = glfwGetMouseButton((GLFWwindow*)m_NativeWindowHandel, mousebutton) == GLFW_RELEASE;
        return released;
    }

    void WindowsPlatformLayer::ExportImagePNG(const std::string& path, uint32_t width, uint32_t height, const std::vector<uint8_t>& pixels, uint32_t bpp)
    {
        auto flippedpixels = FlipTextureVertically(pixels, width, height, bpp);
        // Save pixel data to PNG using stb_image_write
        if (stbi_write_png(path.c_str(), width, height, bpp, flippedpixels.data(), width * bpp))
            std::cout << "Saved framebuffer to " << path << std::endl;
        else
            PIX_ASSERT_MSG(false, "Failed to save image!");
    }

    std::filesystem::path WindowsPlatformLayer::OpenDialogue(FileDialougeFilter Filter)
    {
        bool is_path_dialoge = false;

        OPENFILENAMEA ofn;    // Ansi version of the structure
        CHAR szFile[260] = { 0 };    // Buffer to store the selected file path
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;    // Set the window handle of the parent window if needed
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);

        // Set the filter to show only .proj files
        switch (Filter)
        {
        case FileDialougeFilter::PATH:
        {
            ofn.lpstrFilter = "Path \0*.*\0";
            is_path_dialoge = true;
        }
        break;
        case FileDialougeFilter::PIXSCENE:
            ofn.lpstrFilter = "PIX 3D Scenes (*.pixscene)\0*.pixscene\0All Files (*.*)\0*.*\0"; break;
        case FileDialougeFilter::PNG:
            ofn.lpstrFilter = "Image Files (*.png)\0*.png\0All Files (*.*)\0*.*\0"; break;
        case FileDialougeFilter::PIXPROJ:
            ofn.lpstrFilter = "Pix3D Project Files (*.pixproj)\0*.pixproj\0All Files (*.*)\0*.*\0"; break;
        case FileDialougeFilter::HDR:
            ofn.lpstrFilter = "Image Files (*.hdr)\0*.hdr\0All Files (*.*)\0*.*\0"; break;
        case FileDialougeFilter::GLTF:
            ofn.lpstrFilter = "3D Model Files (*.gltf;*.glb;*.obj)\0*.gltf;*.glb;*.obj\0All Files (*.*)\0*.*\0"; break;
        default:
            break;
        }
        if (!is_path_dialoge)
        {
            // Save the current working directory
            char currentDir[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, currentDir);


            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameA(&ofn) == TRUE)
            {
                // Restore the previous working directory
                SetCurrentDirectoryA(currentDir);
                // Return the selected file path as a std::filesystem::path object
                return std::filesystem::path(szFile);
            }
            else
            {
                // Restore the previous working directory
                SetCurrentDirectoryA(currentDir);
                // Return an empty path if the user canceled the dialog
                return std::filesystem::path();
            }
        }
        else
        {
            BROWSEINFOA bi = { 0 };
            bi.lpszTitle = "Select a Directory";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

            LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
            if (pidl != NULL)
            {
                CHAR path[MAX_PATH];
                if (SHGetPathFromIDListA(pidl, path))
                {
                    CoTaskMemFree(pidl); // Free the memory allocated by SHBrowseForFolderA
                    return std::filesystem::path(path);
                }
                CoTaskMemFree(pidl); // Free the memory even if it fails
            }

            return std::filesystem::path(); // Return an empty path if canceled
        }
    }

    std::filesystem::path WindowsPlatformLayer::SaveDialogue(FileDialougeFilter Filter)
    {
        std::string extension;


        OPENFILENAMEA ofn;    // Ansi version of the structure
        CHAR szFile[260] = { 0 };    // Buffer to store the selected file path
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;    // Set the window handle of the parent window if needed
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);

        // Set the filter to show only .proj files
        switch (Filter)
        {
        case FileDialougeFilter::PIXPROJ:
        {
            ofn.lpstrFilter = "Project Files (*.pixproj)\0*.pixproj\0All Files (*.*)\0*.*\0";
            extension = ".pixproj";
        }break;
        case FileDialougeFilter::PIXSCENE:
        {
            ofn.lpstrFilter = "Project Files (*.pixscene)\0*.pixscene\0All Files (*.*)\0*.*\0";
            extension = ".pixscene";
        }break;
        case FileDialougeFilter::PNG:
        { 
            ofn.lpstrFilter = "Project Files (*.png)\0*.png\0All Files (*.*)\0*.*\0";
            extension = ".png";
        }
        break;
        case FileDialougeFilter::HDR:
        {
            ofn.lpstrFilter = "Project Files (*.hdr)\0*.hdr\0All Files (*.*)\0*.*\0";
            extension = ".hdr";
        }break;
        case FileDialougeFilter::GLTF:
        {
            ofn.lpstrFilter = "Project Files (*.gltf)\0*.gltf\0All Files (*.*)\0*.*\0";
            extension = ".gltf";
        }break;

        default:
            break;
        }


        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        // Save the current working directory
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);

        if (GetSaveFileNameA(&ofn) == TRUE)
        {
            // Restore the previous working directory
            SetCurrentDirectoryA(currentDir);
            // Return the selected file path as a std::filesystem::path object
            auto p = std::filesystem::path(szFile);
            if(p.has_extension())
                return p;
            p += extension;
            return p;
        }
        else
        {
            // Restore the previous working directory
            SetCurrentDirectoryA(currentDir);
            // Return an empty path if the user canceled the dialog
            return std::filesystem::path();
        }
    }

    void WindowsPlatformLayer::ShowCursor(bool show)
    {
        if(show)
            glfwSetInputMode((GLFWwindow*)m_NativeWindowHandel, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode((GLFWwindow*)m_NativeWindowHandel, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}
