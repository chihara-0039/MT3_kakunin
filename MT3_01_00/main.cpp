#include <Novice.h>
#include <algorithm> // std::max, std::min を使用するために必要なヘッダーを追加
#include <cmath>
#include <imgui.h>

const char kWindowTitle[] = "LE2C_21_チハラ_シゴウ";

struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

struct Spheres {
    Vector3 center;
    float radius;
};

struct Segment{
	Vector3 start;
	Vector3 end;
};

struct AABB {
    Vector3 min;
    Vector3 max;
};


// ベクトルを変換
Vector3 Transform(const Vector3& v, const Matrix4x4& m) {
    float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        x /= w; y /= w; z /= w;
    }
    return { x, y, z };
}

// グリッド描画
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix) {
    const float kGridHalfWidth = 2.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

    for (uint32_t i = 0; i <= kSubdivision; ++i) {
        float offset = -kGridHalfWidth + i * kGridEvery;

        // Z方向に平行（X固定）
        Vector3 startX = { offset, 0.0f, -kGridHalfWidth };
        Vector3 endX = { offset, 0.0f, +kGridHalfWidth };

        // X方向に平行（Z固定）
        Vector3 startZ = { -kGridHalfWidth, 0.0f, offset };
        Vector3 endZ = { +kGridHalfWidth, 0.0f, offset };

        Vector3 screenStartX = Transform(Transform(startX, viewProjectionMatrix), viewportMatrix);
        Vector3 screenEndX = Transform(Transform(endX, viewProjectionMatrix), viewportMatrix);
        Vector3 screenStartZ = Transform(Transform(startZ, viewProjectionMatrix), viewportMatrix);
        Vector3 screenEndZ = Transform(Transform(endZ, viewProjectionMatrix), viewportMatrix);

        // 線を描画
        int colorX = (offset == 0.0f) ? 0x000000FF : 0xAAAAAAFF;  // Z方向に平行な線
        int colorZ = (offset == 0.0f) ? 0x000000FF : 0xAAAAAAFF;  // X方向に平行な線

        Novice::DrawLine(int(screenStartX.x), int(screenStartX.y), int(screenEndX.x), int(screenEndX.y), colorX);
        Novice::DrawLine(int(screenStartZ.x), int(screenStartZ.y), int(screenEndZ.x), int(screenEndZ.y), colorZ);

    }
}


// 単位ベクトルの正規化
Vector3 Normalize(const Vector3& v) {
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return { v.x / length, v.y / length, v.z / length };
}

// 外積
Vector3 Cross(const Vector3& a, const Vector3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result = {};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m[i][j] += m1.m[i][k] * m2.m[k][j];
            }
        }
    }
    return result;
}



Matrix4x4 MakeRotateMatrix(const Vector3& rotate) {
    float cosX = cosf(rotate.x), sinX = sinf(rotate.x);
    float cosY = cosf(rotate.y), sinY = sinf(rotate.y);
    float cosZ = cosf(rotate.z), sinZ = sinf(rotate.z);

    Matrix4x4 rotX = {
        1, 0,     0,    0,
        0, cosX,  sinX, 0,
        0, -sinX, cosX, 0,
        0, 0,     0,    1
    };
    Matrix4x4 rotY = {
        cosY, 0, -sinY, 0,
        0,    1, 0,     0,
        sinY, 0, cosY,  0,
        0,    0, 0,     1
    };
    Matrix4x4 rotZ = {
        cosZ, sinZ, 0, 0,
        -sinZ, cosZ, 0, 0,
        0,     0,    1, 0,
        0,     0,    0, 1
    };

    // rotZ * rotX * rotY
    Matrix4x4 result = Multiply(Multiply(rotZ, rotX), rotY);
    return result;
}



// ビュー行列作成（LookAt方式）
Matrix4x4 MakeViewMatrix(const Vector3& eye, const Vector3& target, const Vector3& up) {
    Vector3 zAxis = Normalize({ target.x - eye.x, target.y - eye.y, target.z - eye.z });
    Vector3 xAxis = Normalize(Cross(up, zAxis));
    Vector3 yAxis = Cross(zAxis, xAxis);

    Matrix4x4 viewMatrix = {};
    viewMatrix.m[0][0] = xAxis.x;
    viewMatrix.m[1][0] = xAxis.y;
    viewMatrix.m[2][0] = xAxis.z;
    viewMatrix.m[3][0] = -(eye.x * xAxis.x + eye.y * xAxis.y + eye.z * xAxis.z);

    viewMatrix.m[0][1] = yAxis.x;
    viewMatrix.m[1][1] = yAxis.y;
    viewMatrix.m[2][1] = yAxis.z;
    viewMatrix.m[3][1] = -(eye.x * yAxis.x + eye.y * yAxis.y + eye.z * yAxis.z);

    viewMatrix.m[0][2] = zAxis.x;
    viewMatrix.m[1][2] = zAxis.y;
    viewMatrix.m[2][2] = zAxis.z;
    viewMatrix.m[3][2] = -(eye.x * zAxis.x + eye.y * zAxis.y + eye.z * zAxis.z);

    viewMatrix.m[0][3] = 0;
    viewMatrix.m[1][3] = 0;
    viewMatrix.m[2][3] = 0;
    viewMatrix.m[3][3] = 1;

    return viewMatrix;
}

// 透視射影行列を作成
Matrix4x4 MakePerspectiveMatrix(float fovY, float aspect, float nearZ, float farZ) {
    float f = 1.0f / tanf(fovY / 2.0f);
    Matrix4x4 m{};
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = farZ / (farZ - nearZ);
    m.m[3][2] = -nearZ * farZ / (farZ - nearZ);
    m.m[2][3] = 1.0f;
    m.m[3][3] = 0.0f;
    return m;
}

void DrawSphere(const Spheres& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
    const uint32_t kSubdivision = 16;
    const float pi = 3.1415926535f;
    const float kLatEvery = pi / kSubdivision;
    const float kLonEvery = 2 * pi / kSubdivision;

    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        float lat = -pi / 2.0f + kLatEvery * latIndex;
        float nextLat = lat + kLatEvery;

        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            float lon = lonIndex * kLonEvery;
            float nextLon = lon + kLonEvery;

            Vector3 a = {
                sphere.center.x + sphere.radius * cosf(lat) * cosf(lon),
                sphere.center.y + sphere.radius * sinf(lat),
                sphere.center.z + sphere.radius * cosf(lat) * sinf(lon)
            };
            Vector3 b = {
                sphere.center.x + sphere.radius * cosf(nextLat) * cosf(lon),
                sphere.center.y + sphere.radius * sinf(nextLat),
                sphere.center.z + sphere.radius * cosf(nextLat) * sinf(lon)
            };
            Vector3 c = {
                sphere.center.x + sphere.radius * cosf(lat) * cosf(nextLon),
                sphere.center.y + sphere.radius * sinf(lat),
                sphere.center.z + sphere.radius * cosf(lat) * sinf(nextLon)
            };

            Vector3 screenA = Transform(Transform(a, viewProjectionMatrix), viewportMatrix);
            Vector3 screenB = Transform(Transform(b, viewProjectionMatrix), viewportMatrix);
            Vector3 screenC = Transform(Transform(c, viewProjectionMatrix), viewportMatrix);

            Novice::DrawLine(int(screenA.x), int(screenA.y), int(screenB.x), int(screenB.y), color);
            Novice::DrawLine(int(screenA.x), int(screenA.y), int(screenC.x), int(screenC.y), color);
        }
    }
}

void DrawSegment(const Segment& seg, const Matrix4x4& viewProjection, const Matrix4x4& viewport, uint32_t color) {
    Vector3 screenStart = Transform(Transform(seg.start, viewProjection), viewport);
    Vector3 screenEnd = Transform(Transform(seg.end, viewProjection), viewport);
    Novice::DrawLine((int)screenStart.x, (int)screenStart.y, (int)screenEnd.x, (int)screenEnd.y, color);
}


Vector3 cameraTranslate = { 0.0f, 2.0f, -7.0f };
Vector3 cameraRotate = { 0.0f, 0.0f, 0.0f };
// 球
Spheres sphereA = { {0.0f, 1.0f, 0.0f}, 1.0f };

Segment segment = { {-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f} };





void DrawAABB(const AABB& aabb, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
    Vector3 vertices[8] = {
        {aabb.min.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.min.y, aabb.min.z},
        {aabb.min.x, aabb.max.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.min.z},
        {aabb.min.x, aabb.min.y, aabb.max.z},
        {aabb.max.x, aabb.min.y, aabb.max.z},
        {aabb.min.x, aabb.max.y, aabb.max.z},
        {aabb.max.x, aabb.max.y, aabb.max.z},
    };

    for (int i = 0; i < 8; ++i) {
        vertices[i] = Transform(Transform(vertices[i], viewProjectionMatrix), viewportMatrix);
    }

    int edges[12][2] = {
        {0,1},{1,3},{3,2},{2,0},
        {4,5},{5,7},{7,6},{6,4},
        {0,4},{1,5},{2,6},{3,7}
    };

    for (int i = 0; i < 12; ++i) {
        Novice::DrawLine(
            (int)vertices[edges[i][0]].x, (int)vertices[edges[i][0]].y,
            (int)vertices[edges[i][1]].x, (int)vertices[edges[i][1]].y,
            color
        );
    }
}

bool IsCollision(const Spheres& sphere, const AABB& aabb) {
    float closestX = std::max<float>(aabb.min.x, std::min<float>(sphere.center.x, aabb.max.x));
    float closestY = std::max<float>(aabb.min.y, std::min<float>(sphere.center.y, aabb.max.y));
    float closestZ = std::max<float>(aabb.min.z, std::min<float>(sphere.center.z, aabb.max.z));

    float dx = sphere.center.x - closestX;
    float dy = sphere.center.y - closestY;
    float dz = sphere.center.z - closestZ;

    return (dx * dx + dy * dy + dz * dz) <= (sphere.radius * sphere.radius);
}

// 線分とAABBの衝突判定
bool IsIntersectAABBAndSegment(const AABB& box, const Segment& seg) {
    Vector3 dir = {
        seg.end.x - seg.start.x,
        seg.end.y - seg.start.y,
        seg.end.z - seg.start.z
    };

    float tMin = 0.0f;
    float tMax = 1.0f;

    for (int i = 0; i < 3; ++i) {
        float start = (&seg.start.x)[i];
        float d = (&dir.x)[i];
        float minB = (&box.min.x)[i];
        float maxB = (&box.max.x)[i];

        if (std::abs(d) < 1e-6f) {
            if (start < minB || start > maxB) return false;
        } else {
            float ood = 1.0f / d;
            float t1 = (minB - start) * ood;
            float t2 = (maxB - start) * ood;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max<float>(tMin, t1);
            tMax = std::min<float>(tMax, t2);
            if (tMin > tMax) return false;
        }
    }
    return true;
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, 1280, 720);
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    int mouseX = 0, mouseY = 0;
    int preMouseX = 0, preMouseY = 0;
    bool isRightDragging = false;

    static AABB box = { {-0.5f, 0.0f, -0.5f}, {0.5f, 1.0f, 0.5f} };

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();

        // ImGui 操作パネル
        ImGui::Begin("Window");
        ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
        ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
        ImGui::DragFloat3("SphereCenter", &sphereA.center.x, 0.01f);
        ImGui::DragFloat("SphereA Radius", &sphereA.radius, 0.01f);
        ImGui::DragFloat3("AABB Min", &box.min.x, 0.01f);
        ImGui::DragFloat3("AABB Max", &box.max.x, 0.01f);
        ImGui::DragFloat3("Segment Start", &segment.start.x, 0.01f);
        ImGui::DragFloat3("Segment End", &segment.end.x, 0.01f);



        ImGui::End();


        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        box.min.x = std::min<float>(box.min.x, box.max.x);
        box.max.x = std::max<float>(box.min.x, box.max.x);
        box.min.y = std::min<float>(box.min.y, box.max.y);
        box.max.y = std::max<float>(box.min.y, box.max.y);
        box.min.z = std::min<float>(box.min.z, box.max.z);
        box.max.z = std::max<float>(box.min.z, box.max.z);


        Matrix4x4 rotateMatrix = MakeRotateMatrix(cameraRotate);
        Vector3 forward = Transform({ 0, 0, 1 }, rotateMatrix);
        Vector3 target = {
            cameraTranslate.x + forward.x,
            cameraTranslate.y + forward.y,
            cameraTranslate.z + forward.z
        };
        Vector3 up = Transform({ 0, 1, 0 }, rotateMatrix);

        Matrix4x4 viewMatrix = MakeViewMatrix(cameraTranslate, target, up);



        preMouseX = mouseX;
        preMouseY = mouseY;
        Novice::GetMousePosition(&mouseX, &mouseY);

        if (Novice::IsPressMouse(1)) {
            if (!isRightDragging) {
                isRightDragging = true;
            } else {
                cameraRotate.y += (mouseX - preMouseX) * 0.01f;
                cameraRotate.x += (mouseY - preMouseY) * 0.01f;
            }
        } else {
            isRightDragging = false;
        }


        if (keys[DIK_W]) cameraTranslate.z += 0.1f;
        if (keys[DIK_S]) cameraTranslate.z -= 0.1f;
        if (keys[DIK_A]) cameraTranslate.x -= 0.1f;
        if (keys[DIK_D]) cameraTranslate.x += 0.1f;
        if (keys[DIK_Q]) cameraTranslate.y += 0.1f;
        if (keys[DIK_E]) cameraTranslate.y -= 0.1f;

        // パース付き射影行列
        float fovY = 0.5f;
        float aspect = 1280.0f / 720.0f;
        float nearZ = 0.1f;
        float farZ = 100.0f;
        Matrix4x4 projectionMatrix = MakePerspectiveMatrix(fovY, aspect, nearZ, farZ);

        // viewProjectionMatrix = view × projection
        Matrix4x4 viewProjectionMatrix = {};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                viewProjectionMatrix.m[i][j] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    viewProjectionMatrix.m[i][j] += viewMatrix.m[i][k] * projectionMatrix.m[k][j];
                }
            }
        }

        // ビューポート行列（画面中心に変換）
        Matrix4x4 viewportMatrix = {
            640.0f, 0,       0, 0,
            0,    -360.0f,   0, 0,
            0,       0,      1, 0,
            640.0f, 360.0f,  0, 1
        };


        // 描画
		bool isHit = IsIntersectAABBAndSegment(box, segment);
        int32_t boxColor = isHit ? 0xFF0000FF : 0xFFFFFFFF;
        uint32_t segColor = isHit ? 0xFF0000FF : 0xFFFFFFFF;
        DrawAABB(box, viewProjectionMatrix, viewportMatrix, boxColor);

        //DrawSphere(sphereA, viewProjectionMatrix, viewportMatrix, WHITE);
        DrawGrid(viewProjectionMatrix, viewportMatrix);
        DrawAABB(box, viewProjectionMatrix, viewportMatrix, boxColor);
        DrawSegment(segment, viewProjectionMatrix, viewportMatrix, segColor);



        /// ↑描画処理ここまで
        Novice::EndFrame();

        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    Novice::Finalize();
    return 0;
}