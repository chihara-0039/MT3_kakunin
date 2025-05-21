#include <Novice.h>
#include <cmath>

const char kWindowTitle[] = "LE2C_21_チハラ_シゴウ";

const int kWindowWidth = 1280;
const int kWindowHeight = 720;

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

// ---------- 数学関数 ----------
Vector3 Cross(const Vector3& v1, const Vector3& v2) {
    return {
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    };
}

Matrix4x4 MakeIdentityMatrix() {
    Matrix4x4 mat = {};
    mat.m[0][0] = mat.m[1][1] = mat.m[2][2] = mat.m[3][3] = 1.0f;
    return mat;
}

Matrix4x4 MakeRotateYMatrix(float angle) {
    Matrix4x4 mat = MakeIdentityMatrix();
    mat.m[0][0] = cosf(angle);
    mat.m[0][2] = sinf(angle);
    mat.m[2][0] = -sinf(angle);
    mat.m[2][2] = cosf(angle);
    return mat;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& t) {
    Matrix4x4 mat = MakeIdentityMatrix();
    mat.m[3][0] = t.x;
    mat.m[3][1] = t.y;
    mat.m[3][2] = t.z;
    return mat;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result = {};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            for (int k = 0; k < 4; ++k) {
                result.m[row][col] += m1.m[row][k] * m2.m[k][col];
            }
        }
    }
    return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
    Matrix4x4 s = MakeIdentityMatrix();
    s.m[0][0] = scale.x;
    s.m[1][1] = scale.y;
    s.m[2][2] = scale.z;

    Matrix4x4 ry = MakeRotateYMatrix(rotate.y);
    Matrix4x4 t = MakeTranslateMatrix(translate);

    return Multiply(s, Multiply(ry, t));
}

Matrix4x4 Inverse(const Matrix4x4& m) {
    Matrix4x4 result = MakeIdentityMatrix();
    result.m[3][0] = -m.m[3][0];
    result.m[3][1] = -m.m[3][1];
    result.m[3][2] = -m.m[3][2];
    return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearZ, float farZ) {
    Matrix4x4 mat = {};
    float f = 1.0f / tanf(fovY / 2.0f);
    mat.m[0][0] = f / aspectRatio;
    mat.m[1][1] = f;
    mat.m[2][2] = farZ / (farZ - nearZ);
    mat.m[2][3] = 1.0f;
    mat.m[3][2] = -nearZ * farZ / (farZ - nearZ);
    return mat;
}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
    Matrix4x4 mat = {};
    mat.m[0][0] = width / 2.0f;
    mat.m[1][1] = -height / 2.0f;
    mat.m[2][2] = maxDepth - minDepth;
    mat.m[3][0] = left + width / 2.0f;
    mat.m[3][1] = top + height / 2.0f;
    mat.m[3][2] = minDepth;
    mat.m[3][3] = 1.0f;
    return mat;
}

Vector3 Transform(const Vector3& v, const Matrix4x4& m) {
    Vector3 result;
    result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        result.x /= w;
        result.y /= w;
        result.z /= w;
    }
    return result;
}

//グリッド描画
void DrawGrid(const Matrix4x4& viewProjectMatrix, const Matrix4x4& viewprojectMatrix) {
    const float kGridHalfWidth = 2.0f;                                      //Gridの半分の幅
    const uint32_t kSubdivision = 10;                                       //分割数
    const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision); //一つ分の長さ

    //奥から手前への線を順々に引いていく
    for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
        //  上の情報を使ってワールド座標系上の始点と終点を求める
        float offset = -kGridHalfWidth + xIndex * kGridEvery;
         
        //  スクリーン座標系まで変換をかける
        // Z方向に平行（x固定）
        Vector3 startX = { offset, 0.0f, -kGridHalfWidth };
        Vector3 endX = { offset, 0.0f, +kGridHalfWidth };

        //X方向に平行 (Z固定)
        Vector3 startZ = { -kGridHalfWidth, 0.0f, offset };
        Vector3 endZ = { +kGridHalfWidth, 0.0f, offset };

        Vector3 screenStartX = Transform(Transform(startX, viewProjectionMatrix), viewportMatrix);
        Vector3 screenEndX = Transform(Transform(endX, viewProjectionMatrix), viewportMatrix);
        Vector3 screenStartZ = Transform(Transform(startZ, viewProjectionMatrix), viewportMatrix);
        Vector3 screenEndZ = Transform(Transform(endZ, viewProjectionMatrix), viewportMatrix);

        
        //  変換した座標を使って表示。 色は薄い灰色（0xAAAAAAFF）、原点は黒ぐらいが良いが、何でもいい
        Novice::DrawLine();
    }

    //左から右も同じように順々に引いていく
    for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
        //奥から手前が左右に変わるだけ
    }
}

// ---------- WinMain ----------
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    // 三角形頂点
    Vector3 kLocalVertices[3] = {
        {0.0f, 0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {-0.5f, -0.5f, 0.0f}
    };

    Vector3 translate{ 0.0f, 0.0f, 0.0f };
    float angleY = 0.0f;

    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        // 入力
        if (keys[DIK_W]) translate.z -= 0.1f;
        if (keys[DIK_S]) translate.z += 0.1f;
        if (keys[DIK_A]) translate.x -= 0.1f;
        if (keys[DIK_D]) translate.x += 0.1f;

        // 自動回転
        angleY += 0.03f;

        Vector3 rotate{ 0.0f, angleY, 0.0f };

        // 行列
        Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate, translate);
        Vector3 cameraPosition{ 0.0f, 0.0f, -3.0f };
        Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, {}, cameraPosition);
        Matrix4x4 viewMatrix = Inverse(cameraMatrix);
        Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kWindowWidth) / float(kWindowHeight), 0.1f, 100.0f);
        Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
        Matrix4x4 viewportMatrix = MakeViewportMatrix(0, 0, float(kWindowWidth), float(kWindowHeight), 0.0f, 1.0f);

        // 描画処理
        Vector3 screenVertices[3];
        for (int i = 0; i < 3; ++i) {
            Vector3 ndcVertex = Transform(kLocalVertices[i], worldViewProjectionMatrix);
            screenVertices[i] = Transform(ndcVertex, viewportMatrix);
        }

        Novice::DrawTriangle(
            int(screenVertices[0].x), int(screenVertices[0].y),
            int(screenVertices[1].x), int(screenVertices[1].y),
            int(screenVertices[2].x), int(screenVertices[2].y),
            RED, kFillModeSolid
        );

        // クロス積の表示
        Vector3 v1{ 1.2f, -3.9f, 2.5f };
        Vector3 v2{ 2.8f, 8.4f, -1.3f };
        Vector3 cross = Cross(v1, v2);
        Novice::ScreenPrintf(0, 0, "Cross: x=%.2f y=%.2f z=%.2f", cross.x, cross.y, cross.z);

        Novice::EndFrame();

        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    Novice::Finalize();
    return 0;
}
