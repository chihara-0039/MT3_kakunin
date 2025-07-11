#include <Novice.h>
#include <cmath>
#include <imgui.h>
#include <algorithm>

const char kWindowTitle[] = "LE2D_26_ワカマツ_サンガ";

struct Matrix4x4 {
    float m[4][4];
};

struct Vector3 {
    float x, y, z;
};

struct Sphere {
    Vector3 center;
    float radius;
};

struct AABB {
    Vector3 min;
    Vector3 max;
};

float Dot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 Transform(const Vector3& v, const Matrix4x4& m) {
    float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (std::fabs(w) < 1e-5f) w = 1.0f;
    return { x / w, y / w, z / w };
}

Matrix4x4 MakeIdentity() {
    Matrix4x4 result{};
    for (int i = 0; i < 4; ++i) result.m[i][i] = 1.0f;
    return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& t) {
    Matrix4x4 result = MakeIdentity();
    result.m[3][0] = t.x;
    result.m[3][1] = t.y;
    result.m[3][2] = t.z;
    return result;
}

Matrix4x4 MakeRotateX(float angle) {
    Matrix4x4 result = MakeIdentity();
    result.m[1][1] = cosf(angle);
    result.m[1][2] = sinf(angle);
    result.m[2][1] = -sinf(angle);
    result.m[2][2] = cosf(angle);
    return result;
}

Matrix4x4 MakeRotateY(float angle) {
    Matrix4x4 result = MakeIdentity();
    result.m[0][0] = cosf(angle);
    result.m[0][2] = -sinf(angle);
    result.m[2][0] = sinf(angle);
    result.m[2][2] = cosf(angle);
    return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                result.m[i][j] += m1.m[i][k] * m2.m[k][j];
    return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearZ, float farZ) {
    Matrix4x4 result{};
    float f = 1.0f / tanf(fovY / 2.0f);
    result.m[0][0] = f / aspect;
    result.m[1][1] = f;
    result.m[2][2] = farZ / (farZ - nearZ);
    result.m[2][3] = 1.0f;
    result.m[3][2] = -nearZ * farZ / (farZ - nearZ);
    return result;
}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
    Matrix4x4 result = MakeIdentity();
    result.m[0][0] = width / 2.0f;
    result.m[1][1] = -height / 2.0f;
    result.m[2][2] = maxDepth - minDepth;
    result.m[3][0] = left + width / 2.0f;
    result.m[3][1] = top + height / 2.0f;
    result.m[3][2] = minDepth;
    return result;
}

// Clamp関数
float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// AABBと球体の衝突判定
bool IsCollision(const AABB& aabb, const Sphere& sphere) {
    float x = Clamp(sphere.center.x, aabb.min.x, aabb.max.x);
    float y = Clamp(sphere.center.y, aabb.min.y, aabb.max.y);
    float z = Clamp(sphere.center.z, aabb.min.z, aabb.max.z);
    float dx = x - sphere.center.x;
    float dy = y - sphere.center.y;
    float dz = z - sphere.center.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    return distSq <= sphere.radius * sphere.radius;
}

// グリッド描画
void DrawGrid(const Matrix4x4& vp, const Matrix4x4& viewport) {
    float half = 2.0f;
    for (int i = -10; i <= 10; ++i) {
        float pos = i * 0.2f;
        Vector3 startX = Transform(Transform({ pos, 0, -half }, vp), viewport);
        Vector3 endX = Transform(Transform({ pos, 0, half }, vp), viewport);
        Vector3 startZ = Transform(Transform({ -half, 0, pos }, vp), viewport);
        Vector3 endZ = Transform(Transform({ half, 0, pos }, vp), viewport);
        uint32_t color = (i == 0) ? 0x000000FF : 0xAAAAAAFF;
        Novice::DrawLine((int)startX.x, (int)startX.y, (int)endX.x, (int)endX.y, color);
        Novice::DrawLine((int)startZ.x, (int)startZ.y, (int)endZ.x, (int)endZ.y, color);
    }
}

// 球体描画（ワイヤーフレーム）
void DrawSphere(const Sphere& sphere, const Matrix4x4& vp, const Matrix4x4& viewport, uint32_t color) {
    const int kDiv = 12;
    const float pi = 3.141592f;
    for (int i = 0; i < kDiv; ++i) {
        float lat1 = pi * (-0.5f + (float)i / kDiv);
        float lat2 = pi * (-0.5f + (float)(i + 1) / kDiv);
        for (int j = 0; j < kDiv; ++j) {
            float lon1 = 2 * pi * (float)(j) / kDiv;
            float lon2 = 2 * pi * (float)(j + 1) / kDiv;

            Vector3 a = {
                sphere.center.x + sphere.radius * cosf(lat1) * cosf(lon1),
                sphere.center.y + sphere.radius * sinf(lat1),
                sphere.center.z + sphere.radius * cosf(lat1) * sinf(lon1)
            };
            Vector3 b = {
                sphere.center.x + sphere.radius * cosf(lat2) * cosf(lon1),
                sphere.center.y + sphere.radius * sinf(lat2),
                sphere.center.z + sphere.radius * cosf(lat2) * sinf(lon1)
            };
            Vector3 c = {
                sphere.center.x + sphere.radius * cosf(lat1) * cosf(lon2),
                sphere.center.y + sphere.radius * sinf(lat1),
                sphere.center.z + sphere.radius * cosf(lat1) * sinf(lon2)
            };

            a = Transform(Transform(a, vp), viewport);
            b = Transform(Transform(b, vp), viewport);
            c = Transform(Transform(c, vp), viewport);

            Novice::DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, color);
            Novice::DrawLine((int)a.x, (int)a.y, (int)c.x, (int)c.y, color);
        }
    }
}

// AABB描画
void DrawAABB(const AABB& aabb, const Matrix4x4& vp, const Matrix4x4& viewport, uint32_t color) {
    Vector3 c[8] = {
        {aabb.min.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.min.z},
        {aabb.min.x, aabb.max.y, aabb.min.z},
        {aabb.min.x, aabb.min.y, aabb.max.z},
        {aabb.max.x, aabb.min.y, aabb.max.z},
        {aabb.max.x, aabb.max.y, aabb.max.z},
        {aabb.min.x, aabb.max.y, aabb.max.z},
    };
    for (int i = 0; i < 8; ++i) c[i] = Transform(Transform(c[i], vp), viewport);
    int edges[12][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7} };
    for (int i = 0; i < 12; ++i) {
        Novice::DrawLine((int)c[edges[i][0]].x, (int)c[edges[i][0]].y,
                         (int)c[edges[i][1]].x, (int)c[edges[i][1]].y, color);
    }
}

// メイン関数
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256]{}, preKeys[256]{};

    Vector3 cameraTranslate{ 0.0f, 1.9f, -6.49f };
    Vector3 cameraRotate{ 0.26f, 0.0f, 0.0f };
    Sphere sphere{ {0.0f, 0.5f, 0.0f}, 0.5f };
    AABB box{ {-0.5f, -0.5f, -0.5f}, {0.5f, 0.0f, 0.5f} };

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        Matrix4x4 view = Multiply(
            MakeTranslateMatrix({ -cameraTranslate.x, -cameraTranslate.y, -cameraTranslate.z }),
            Multiply(MakeRotateY(-cameraRotate.y), MakeRotateX(-cameraRotate.x))
        );
        Matrix4x4 proj = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
        Matrix4x4 vp = Multiply(view, proj);
        Matrix4x4 viewport = MakeViewportMatrix(0, 0, 1280, 720, 0.0f, 1.0f);

        // ImGui操作
        ImGui::Begin("Controls");
        ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
        ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
        ImGui::SeparatorText("Sphere");
        ImGui::DragFloat3("Center", &sphere.center.x, 0.01f);
        ImGui::DragFloat("Radius", &sphere.radius, 0.01f);
        ImGui::SeparatorText("AABB");
        ImGui::DragFloat3("Min", &box.min.x, 0.01f);
        ImGui::DragFloat3("Max", &box.max.x, 0.01f);
        ImGui::End();

        // 衝突判定
        bool hit = IsCollision(box, sphere);

        // 描画
        DrawGrid(vp, viewport);
        DrawAABB(box, vp, viewport, 0xFFFFFFFF);
        DrawSphere(sphere, vp, viewport, hit ? 0xFF0000FF : 0x000000FF);

        Novice::EndFrame();
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) break;
    }

    Novice::Finalize();
    return 0;
}