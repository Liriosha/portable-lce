R "GLSL(
#version 300 es
precision mediump float;
precision mediump int;

uniform sampler2D uTex0;
uniform sampler2D uTex1;
uniform int uUseTexture;
uniform int uUseTileUV;
uniform int uUseLightmap;
uniform float uAlphaRef;
uniform vec4 uFogColor;
uniform int uFogEnable;
uniform float uInvGamma;

in vec2 vUV0;
in vec2 vUV1;
in vec4 vColor;
in vec3 vWorldPos;
in float vFogFactor;
out vec4 oColor;

void main() {
    vec2 uv = vUV0;
    vec2 dUVdx = vec2(0.0);
    vec2 dUVdy = vec2(0.0);
    bool useGrad = false;
    bool disableMipmap = false;

    if (uUseTileUV != 0) {
        vec2 baseUV = vUV0;
        if (baseUV.x > 1.0) {
            baseUV.x -= 1.0;
            disableMipmap = true;
        }
        if (baseUV.y > 1.0) {
            baseUV.y -= 1.0;
            disableMipmap = true;
        }

        vec2 atlasSize = vec2(textureSize(uTex0, 0));
        float tileSize = 16.0;
        vec2 cell = floor(baseUV * atlasSize / tileSize);
        vec2 uv0 = cell * tileSize / atlasSize;
        vec2 cellSize = vec2(tileSize) / atlasSize;

        vec3 dpdx = dFdx(vWorldPos);
        vec3 dpdy = dFdy(vWorldPos);
        vec3 n = normalize(cross(dpdx, dpdy));

        vec3 uAxis;
        vec3 vAxis;
        if (abs(n.y) > abs(n.x) && abs(n.y) > abs(n.z)) {
            uAxis = vec3(1.0, 0.0, 0.0);
            vAxis = vec3(0.0, 0.0, 1.0);
        } else if (abs(n.z) > abs(n.x)) {
            uAxis = vec3(1.0, 0.0, 0.0);
            vAxis = vec3(0.0, 1.0, 0.0);
        } else {
            uAxis = vec3(0.0, 0.0, 1.0);
            vAxis = vec3(0.0, 1.0, 0.0);
        }

        float du = dot(vWorldPos, uAxis);
        float dv = dot(vWorldPos, vAxis);

        vec2 gradU = vec2(dot(dpdx, uAxis), dot(dpdy, uAxis));
        vec2 gradV = vec2(dot(dpdx, vAxis), dot(dpdy, vAxis));
        vec2 gradUx = vec2(dFdx(baseUV.x), dFdy(baseUV.x));
        vec2 gradUy = vec2(dFdx(baseUV.y), dFdy(baseUV.y));

        float corrUx = dot(gradUx, gradU);
        float corrVx = dot(gradUx, gradV);
        bool swap = abs(corrVx) > abs(corrUx);

        float signU = swap ? corrVx : corrUx;
        float corrUy = dot(gradUy, gradU);
        float corrVy = dot(gradUy, gradV);
        float signV = swap ? corrUy : corrVy;

        float tu = swap ? dv : du;
        float tv = swap ? du : dv;

        float dtu_dx = swap ? dot(dpdx, vAxis) : dot(dpdx, uAxis);
        float dtu_dy = swap ? dot(dpdy, vAxis) : dot(dpdy, uAxis);
        float dtv_dx = swap ? dot(dpdx, uAxis) : dot(dpdx, vAxis);
        float dtv_dy = swap ? dot(dpdy, uAxis) : dot(dpdy, vAxis);

        if (signU < 0.0) {
            tu = -tu;
            dtu_dx = -dtu_dx;
            dtu_dy = -dtu_dy;
        }
        if (signV < 0.0) {
            tv = -tv;
            dtv_dx = -dtv_dx;
            dtv_dy = -dtv_dy;
        }

        uv = uv0 + fract(vec2(tu, tv)) * cellSize;
        dUVdx = vec2(dtu_dx, dtv_dx) * cellSize;
        dUVdy = vec2(dtu_dy, dtv_dy) * cellSize;
        useGrad = true;
    }

    vec4 texColor = vec4(1.0);
    if (uUseTexture != 0) {
        if (useGrad) {
            texColor = disableMipmap ? textureLod(uTex0, uv, 0.0) : textureGrad(uTex0, uv, dUVdx, dUVdy);
        } else {
            texColor = texture(uTex0, uv);
        }
    }
    vec4 c = texColor * vColor;
    if (c.a < uAlphaRef) discard;
    if (uUseLightmap != 0) c.rgb *= texture(uTex1, vUV1).rgb;
    if (uFogEnable != 0) c.rgb = mix(uFogColor.rgb, c.rgb, vFogFactor);

    c.rgb = pow(c.rgb, vec3(uInvGamma));

    oColor = c;
}
) GLSL ";

