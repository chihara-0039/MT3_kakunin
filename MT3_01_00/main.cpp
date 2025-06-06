#include <Novice.h>
#include <cmath>
#include <imgui.h>
#define NOMINMAX
#include <Windows.h>
#include <algorithm>

// ウィンドウタイトル
const char kWindowTitle[] = "LE2C_21_チハラ_シゴウ";

// 3次元ベクトル
struct Vector3 {
	float x, y, z;
};

// 4x4 行列
struct Matrix4x4 {
	float m[4][4];
};

// 球体（中心と半径）
struct Sphere {
	Vector3 center;
	float radius;
};

// 線分（始点と差分ベクトル）
struct Segment {
	Vector3 origin;
	Vector3 diff;
};

// ベクトルを行列で変換（w除算あり）
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

// 地面のグリッドを描画
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix) {
	const float kGridHalfWidth = 2.0f;
	const uint32_t kSubdivision = 10;
	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

	for (uint32_t i = 0; i <= kSubdivision; ++i) {
		float offset = -kGridHalfWidth + i * kGridEvery;

		// 線分の始点と終点（X軸平行とZ軸平行）
		Vector3 startX = { offset, 0.0f, -kGridHalfWidth };
		Vector3 endX = { offset, 0.0f, +kGridHalfWidth };
		Vector3 startZ = { -kGridHalfWidth, 0.0f, offset };
		Vector3 endZ = { +kGridHalfWidth, 0.0f, offset };

		// スクリーン座標に変換
		Vector3 screenStartX = Transform(Transform(startX, viewProjectionMatrix), viewportMatrix);
		Vector3 screenEndX = Transform(Transform(endX, viewProjectionMatrix), viewportMatrix);
		Vector3 screenStartZ = Transform(Transform(startZ, viewProjectionMatrix), viewportMatrix);
		Vector3 screenEndZ = Transform(Transform(endZ, viewProjectionMatrix), viewportMatrix);

		// 中心線は黒、それ以外はグレー
		int colorX = (offset == 0.0f) ? 0x000000FF : 0xAAAAAAFF;
		int colorZ = (offset == 0.0f) ? 0x000000FF : 0xAAAAAAFF;

		// 線の描画
		Novice::DrawLine(int(screenStartX.x), int(screenStartX.y), int(screenEndX.x), int(screenEndX.y), colorX);
		Novice::DrawLine(int(screenStartZ.x), int(screenStartZ.y), int(screenEndZ.x), int(screenEndZ.y), colorZ);
	}
}

// ベクトルの正規化
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

// 4x4行列同士の乗算
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

// ベクトルの減算・加算・スカラー倍
Vector3 Subtract(const Vector3& a, const Vector3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vector3 Add(const Vector3& a, const Vector3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vector3 Multiply(const Vector3& v, float scalar) { return { v.x * scalar, v.y * scalar, v.z * scalar }; }

// 内積
float Dot(const Vector3& a, const Vector3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// ベクトルv1をv2に射影
Vector3 Project(const Vector3& v1, const Vector3& v2) {
	float dotVV = Dot(v2, v2);
	if (dotVV == 0.0f) return { 0, 0, 0 };
	float t = Dot(v1, v2) / dotVV;
	return Multiply(v2, t);
}

// 点から線分への最近接点を求める
Vector3 ClosestPoint(const Vector3& point, const Segment& segment) {
	Vector3 toPoint = Subtract(point, segment.origin);
	float t = Dot(toPoint, segment.diff) / Dot(segment.diff, segment.diff);
	t = (std::max)(0.0f, (std::min)(1.0f, t));  // 線分内に制限
	return Add(segment.origin, Multiply(segment.diff, t));
}

// 回転行列（XYZ回転）
Matrix4x4 MakeRotateMatrix(const Vector3& rotate) {
	// 各軸の回転行列を生成
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

	// rotZ * rotX * rotYの順で合成
	return Multiply(Multiply(rotZ, rotX), rotY);
}

// ビュー行列（LookAt方式）
Matrix4x4 MakeViewMatrix(const Vector3& eye, const Vector3& target, const Vector3& up) {
	Vector3 zAxis = Normalize({ target.x - eye.x, target.y - eye.y, target.z - eye.z });
	Vector3 xAxis = Normalize(Cross(up, zAxis));
	Vector3 yAxis = Cross(zAxis, xAxis);

	Matrix4x4 viewMatrix = {};
	viewMatrix.m[0][0] = xAxis.x;
	viewMatrix.m[1][0] = xAxis.y;
	viewMatrix.m[2][0] = xAxis.z;
	viewMatrix.m[3][0] = -Dot(eye, xAxis);

	viewMatrix.m[0][1] = yAxis.x;
	viewMatrix.m[1][1] = yAxis.y;
	viewMatrix.m[2][1] = yAxis.z;
	viewMatrix.m[3][1] = -Dot(eye, yAxis);

	viewMatrix.m[0][2] = zAxis.x;
	viewMatrix.m[1][2] = zAxis.y;
	viewMatrix.m[2][2] = zAxis.z;
	viewMatrix.m[3][2] = -Dot(eye, zAxis);

	viewMatrix.m[0][3] = 0;
	viewMatrix.m[1][3] = 0;
	viewMatrix.m[2][3] = 0;
	viewMatrix.m[3][3] = 1;

	return viewMatrix;
}

// 透視投影行列の作成
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

// 球を画面上に描画
void DrawSphere(const Sphere& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, int color) {
	Vector3 screen = Transform(Transform(sphere.center, viewProjectionMatrix), viewportMatrix);
	Novice::DrawEllipse(int(screen.x), int(screen.y), int(sphere.radius * 720), int(sphere.radius * 720), 0.0f, color, kFillModeSolid);
}

// カメラ位置と回転
Vector3 cameraTranslate = { 0.0f, 2.0f, -7.0f };
Vector3 cameraRotate = { 0.0f, 0.0f, 0.0f };

// エントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// Novice初期化
	Novice::Initialize(kWindowTitle, 1280, 720);

	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();

		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		// カメラの回転行列を作成
		Matrix4x4 rotateMatrix = MakeRotateMatrix(cameraRotate);
		Vector3 forward = Transform({ 0, 0, 1 }, rotateMatrix);
		Vector3 target = Add(cameraTranslate, forward);
		Vector3 up = Transform({ 0, 1, 0 }, rotateMatrix);

		// ビュー行列と射影行列
		Matrix4x4 viewMatrix = MakeViewMatrix(cameraTranslate, target, up);
		Matrix4x4 projectionMatrix = MakePerspectiveMatrix(0.5f, 1280.0f / 720.0f, 0.1f, 100.0f);

		// view × projection
		Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);

		// ビューポート変換
		Matrix4x4 viewportMatrix = {
			640.0f, 0,       0, 0,
			0,    -360.0f,   0, 0,
			0,       0,      1, 0,
			640.0f, 360.0f,  0, 1
		};

		// 線分と点を定義
		Segment segment{ {-2.0f,-1.0f, 0.0f}, {3.0f, 2.0f, 2.0f} };
		Vector3 point{ -1.5f, 0.6f, 0.6f };

		// 点から線分への射影・最近接点
		Vector3 project = Project(Subtract(point, segment.origin), segment.diff);
		Vector3 closestPoint = ClosestPoint(point, segment);

		// 球の描画
		DrawSphere({ point, 0.01f }, viewProjectionMatrix, viewportMatrix, 0xFF0000FF);       // 点
		DrawSphere({ closestPoint, 0.01f }, viewProjectionMatrix, viewportMatrix, 0x000000FF); // 最近接点

		// 線分の描画
		Vector3 start = Transform(Transform(segment.origin, viewProjectionMatrix), viewportMatrix);
		Vector3 end = Transform(Transform(Add(segment.origin, segment.diff), viewProjectionMatrix), viewportMatrix);
		Novice::DrawLine(int(start.x), int(start.y), int(end.x), int(end.y), 0xFFFFFFFF);

		// ImGui UI（カメラ操作 + デバッグ表示）
		ImGui::Begin("Window");
		ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
		ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
		ImGui::InputFloat3("Project", &project.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::End();

		// グリッド描画
		DrawGrid(viewProjectionMatrix, viewportMatrix);

		Novice::EndFrame();

		// ESCキーで終了
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	Novice::Finalize();
	return 0;
}
