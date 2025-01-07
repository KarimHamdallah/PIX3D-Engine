#pragma once

#include <iostream>
#include <chrono>

namespace PIX3D
{
    class Timer
    {
    public:
        Timer(const std::string& name)
            : m_Start(std::chrono::high_resolution_clock::now()), m_Name(name)
        {}

        ~Timer()
        {
            std::cout << m_Name << " - Elapsed time: " << ElapsedMilliseconds() << " ms\n";
        }

    private:
        double ElapsedMilliseconds() const
        {
            auto end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<double, std::milli>(end - m_Start).count();
        }

        double ElapsedSeconds() const
        {
            auto end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<double>(end - m_Start).count();
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
        std::string m_Name;
    };
}
