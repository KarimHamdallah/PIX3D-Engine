#version 460 core

layout (location = 0) out vec4 FragColor;

// in
layout (location = 0) in vec2 in_coords;
layout (location = 1) in vec4 in_color;
layout (location = 2) in float in_circle_quad;

void main()
{
   vec4 color = in_color;

   if(in_circle_quad == 0.0)
   {
      // Compute distance from the center
      float distanceFromCenter = length(in_coords);
      
      if(distanceFromCenter >= 1.0) discard;
    }

    FragColor = color;
}
