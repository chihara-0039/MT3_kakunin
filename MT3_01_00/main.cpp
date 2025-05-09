#include <Novice.h>
#include <cmath>
const char kWindowTitle[] = "LE2C_チハラ_シゴウ";

struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

// 表示関数
int kRowHeight = 20;
int kColumnWidth = 60;
void MatrixScreenPrintf(int x, int y, const Matrix4x4& matirix,
                        const char* label) {
    Novice::ScreenPrintf(x, y, "%s", label);
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            Novice::ScreenPrintf(x + column * kColumnWidth,
                                 y + (row + 1) * kRowHeight, "%6.02f",
                                 matirix.m[row][column]);
        }
    }
}

// 表示関数
// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {
    Matrix4x4 result = {};

    float f = 1.0f / std::tan(fovY / 2.0f);

    result.m[0][0] = f / aspectRatio; // ここ疑問
    result.m[1][1] = f;
    result.m[2][2] = farClip / (farClip - nearClip);
    result.m[2][3] = 1.0f;
    result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);

    return result;
}

// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip) {
    Matrix4x4 m = {};

    m.m[0][0] = 2.0f / (right - left);
    m.m[1][1] = 2.0f / (top - bottom);
    m.m[2][2] = 1.0f / (farClip - nearClip);
    m.m[3][0] = -(right + left) / (right - left);
    m.m[3][1] = -(top + bottom) / (top - bottom);
    m.m[3][2] = -nearClip / (farClip - nearClip);
    m.m[3][3] = 1.0f;

    return m;
}

// ビューポート変換行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height,
                             float minDepth, float maxDepth) {
    Matrix4x4 m = {};

    // 行0：X方向スケーリングと移動
    m.m[0][0] = width / 2.0f;
    m.m[3][0] = left + width / 2.0f;
    m.m[1][1] = -height / 2.0f;
    m.m[3][1] = top + height / 2.0f;
    m.m[2][2] = maxDepth - minDepth;
    m.m[2][3] = minDepth;
    m.m[3][3] = 1.0f;

    return m;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, 1280, 720);

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓更新処理ここから
        ///
        
        Matrix4x4 orthographicMatrix =
            MakeOrthographicMatrix(-160.0f, 160.0f, 200.0f, 300.0f, 0.0f, 1000.0f);
        
        Matrix4x4 perspectiveFovMatrix =
            MakePerspectiveFovMatrix(0.63f, 1.33f, 0.1f, 1000.0f);
       
        Matrix4x4 viewportMatrix =
            MakeViewportMatrix(100.0f, 200.0f, 600.0f, 300.0f, 0.0f, 1.0f);
        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///
        
        MatrixScreenPrintf(0, 0, orthographicMatrix, "orthographicMatrix");
        MatrixScreenPrintf(0, kRowHeight * 5, perspectiveFovMatrix,
                           "oerspectiveFovMatrix");
       
        MatrixScreenPrintf(0, kRowHeight * 10, viewportMatrix, "vieportMatrix");
        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}