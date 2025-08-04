#include <Novice.h>
#include <imgui.h>
#include <cmath>

const char kWindowTitle[] = "LE2C_21_チハラ_シゴウ";

struct Vector3 {
    float x, y, z;
    Vector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
    Vector3 operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
    Vector3 operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
};

struct Matrix4x4 {
    float m[4][4];
    Matrix4x4() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                m[i][j] = (i == j) ? 1.0f : 0.0f;
    }
    Matrix4x4 operator*(const Matrix4x4& rhs) const {
        Matrix4x4 R;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                R.m[i][j] = 0;
                for (int k = 0; k < 4; k++) {
                    R.m[i][j] += m[i][k] * rhs.m[k][j];
                }
            }
        }
        return R;
    }
};

Matrix4x4 MakeRotateXMatrix(float rad) {
    Matrix4x4 r;
    float c = cosf(rad);
    float s = sinf(rad);
    r.m[1][1] = c; r.m[1][2] = -s;
    r.m[2][1] = s; r.m[2][2] = c;
    return r;
}

Matrix4x4 MakeRotateYMatrix(float rad) {
    Matrix4x4 r;
    float c = cosf(rad);
    float s = sinf(rad);
    r.m[0][0] = c;  r.m[0][2] = s;
    r.m[2][0] = -s; r.m[2][2] = c;
    return r;
}

Matrix4x4 MakeRotateZMatrix(float rad) {
    Matrix4x4 r;
    float c = cosf(rad);
    float s = sinf(rad);
    r.m[0][0] = c; r.m[0][1] = -s;
    r.m[1][0] = s; r.m[1][1] = c;
    return r;
}

void ShowMatrix(const char* label, const Matrix4x4& mat) {
    ImGui::Text("%s", label);
    for (int i = 0; i < 4; i++) {
        ImGui::Text("| %.6f %.6f %.6f %.6f |",
            mat.m[i][0], mat.m[i][1], mat.m[i][2], mat.m[i][3]);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ImGui::NewFrame();

        Vector3 a{ 0.2f, 1.0f, 0.0f };
        Vector3 b{ 2.4f, 3.1f, 1.2f };
        Vector3 c = a + b;
        Vector3 d = a - b;
        Vector3 e = a * 2.4f;

        Vector3 rotate{ -0.4f, -1.43f, 0.8f }; // 正確な回転角（ラジアン）
        Matrix4x4 rotX = MakeRotateXMatrix(rotate.x);
        Matrix4x4 rotY = MakeRotateYMatrix(rotate.y);
        Matrix4x4 rotZ = MakeRotateZMatrix(rotate.z);
        Matrix4x4 rot = rotX * rotY * rotZ;

        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Always);
        ImGui::Begin("Window");
        ImGui::Text("c: %.6f, %.6f, %.6f", c.x, c.y, c.z);
        ImGui::Text("d: %.6f, %.6f, %.6f", d.x, d.y, d.z);
        ImGui::Text("e: %.6f, %.6f, %.6f", e.x, e.y, e.z);
        ShowMatrix("rotateMatrix:", rot);
        ImGui::End();

        Novice::EndFrame();
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) break;
    }

    Novice::Finalize();
    return 0;
}
