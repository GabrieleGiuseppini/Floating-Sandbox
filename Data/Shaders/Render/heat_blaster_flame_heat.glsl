#include "heat_blaster_flame.glslinc"

void main()
{         
    float radius1 = GetFlameRadius(1.0);
    
    // Focus (compress dynamics)
    float radius2 = smoothstep(0.2, 0.35, radius1);
    
    // Transparent when radius1 >= 0.35, opaque when radius <= 0.2
    float alpha = 1.0 - radius2;

    // Core1 (white->yellow)
    vec3 col1 = mix(vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 0.1), smoothstep(0.03, 0.1, radius1));

    // Core2 (->red)
    col1 = mix(col1, vec3(1.0, 0.35, 0.0), smoothstep(0.12, 0.23, radius1));

    // Border (->dark red)
    col1 = mix(col1, vec3(0.3, 0.0, 0.0), smoothstep(0.2, 0.23, radius1));

    gl_FragColor = vec4(col1.xyz, alpha);
}