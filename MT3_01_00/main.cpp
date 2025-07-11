#include <Novice.h>
#include <cmath>
#include <imgui.h>
#include <algorithm>

const char kWindowTitle[] = "LE2C_21_チハラ_シゴウ";

struct Vector3 {
	float x, y, z;
};

struct Matrix4x4 {
	float m[4][4];
};

struct AABB {
	Vector3 min;
	Vector3 max;
};

// 衝突判定関数
bool IsCollision(const AABB& aabb1, const AABB& aabb2) {
	return (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
		(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
		(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z);
}

// 単純な行列演算・ベクトル関数
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

Vector3 Transform(const Vector3& v, const Matrix4x4& m) {
	float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
	float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
	float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
	float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
	return { x / w, y / w, z / w };
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearZ, float farZ) {
	Matrix4x4 result{};
	float f = 1.0f / tanf(fovY / 2.0f);
	result.m[0][0] = f / aspect;
	result.m[1][1] = f;
	result.m[2][2] = farZ / (farZ - nearZ);
	result.m[2][3] = 1.0f;
	result.m[3][2] = (-nearZ * farZ) / (farZ - nearZ);
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

// グリッド描画
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix) {
	const float kGridHalfWidth = 2.0f;
	const uint32_t kSubdivision = 10;
	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

	for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
		float x = -kGridHalfWidth + xIndex * kGridEvery;
		Vector3 start = Transform({ x, 0.0f, -kGridHalfWidth }, viewProjectionMatrix);
		Vector3 end = Transform({ x, 0.0f, kGridHalfWidth }, viewProjectionMatrix);
		start = Transform(start, viewportMatrix);
		end = Transform(end, viewportMatrix);
		uint32_t color = (fabsf(x) < 0.001f) ? 0x000000FF : 0xAAAAAAFF;
		Novice::DrawLine(int(start.x), int(start.y), int(end.x), int(end.y), color);
	}

	for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
		float z = -kGridHalfWidth + zIndex * kGridEvery;
		Vector3 start = Transform({ -kGridHalfWidth, 0.0f, z }, viewProjectionMatrix);
		Vector3 end = Transform({ kGridHalfWidth, 0.0f, z }, viewProjectionMatrix);
		start = Transform(start, viewportMatrix);
		end = Transform(end, viewportMatrix);
		uint32_t color = (fabsf(z) < 0.001f) ? 0x000000FF : 0xAAAAAAFF;
		Novice::DrawLine(int(start.x), int(start.y), int(end.x), int(end.y), color);
	}
}

void DrawAABB(const AABB& box, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
	Vector3 corners[8] = {
		box.min,
		{box.max.x, box.min.y, box.min.z},
		{box.min.x, box.max.y, box.min.z},
		{box.min.x, box.min.y, box.max.z},
		{box.max.x, box.max.y, box.min.z},
		{box.max.x, box.min.y, box.max.z},
		{box.min.x, box.max.y, box.max.z},
		box.max
	};

	for (int i = 0; i < 8; ++i) {
		corners[i] = Transform(corners[i], viewProjectionMatrix);
		corners[i] = Transform(corners[i], viewportMatrix);
	}

	int edges[12][2] = {
		{0,1}, {1,4}, {4,2}, {2,0},
		{3,5}, {5,7}, {7,6}, {6,3},
		{0,3}, {1,5}, {2,6}, {4,7}
	};

	for (auto& e : edges) {
		Novice::DrawLine((int)corners[e[0]].x, (int)corners[e[0]].y,
			(int)corners[e[1]].x, (int)corners[e[1]].y, color);
	}
}

// === Main関数 ===
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	Novice::Initialize(kWindowTitle, 1280, 720);
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	Vector3 cameraTranslate{ 0.0f, 1.9f, -6.49f };
	Vector3 cameraRotate{ 0.26f, 0.0f, 0.0f };

	// AABB 2つ
	AABB aabb1{
		.min{-0.5f, -0.5f, -0.5f},
		.max{ 0.0f,  0.0f,  0.0f}
	};
	AABB aabb2{
		.min{ 0.2f,  0.2f,  0.2f},
		.max{ 1.0f,  1.0f,  1.0f}
	};

	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		Matrix4x4 worldMatrix = MakeIdentity();
		Matrix4x4 cameraMatrix = Multiply(Multiply(MakeRotateX(cameraRotate.x), MakeRotateY(cameraRotate.y)), MakeTranslateMatrix(cameraTranslate));
		Matrix4x4 viewMatrix = cameraMatrix;
		viewMatrix.m[3][0] *= -1.0f;
		viewMatrix.m[3][1] *= -1.0f;
		viewMatrix.m[3][2] *= -1.0f;
		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
		Matrix4x4 viewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
		Matrix4x4 viewportMatrix = MakeViewportMatrix(0, 0, 1280, 720, 0.0f, 1.0f);

		// ImGui操作
		ImGui::Begin("Control");
		ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
		ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
		ImGui::DragFloat3("AABB1.min", &aabb1.min.x, 0.01f);
		ImGui::DragFloat3("AABB1.max", &aabb1.max.x, 0.01f);
		ImGui::DragFloat3("AABB2.min", &aabb2.min.x, 0.01f);
		ImGui::DragFloat3("AABB2.max", &aabb2.max.x, 0.01f);
		ImGui::End();

		// aabb1のminとmaxの順序を保証（負のサイズを防ぐ）
		aabb1.min.x = (std::min)(aabb1.min.x, aabb1.max.x);
		aabb1.max.x = (std::max)(aabb1.min.x, aabb1.max.x);
		aabb1.min.y = (std::min)(aabb1.min.y, aabb1.max.y);
		aabb1.max.y = (std::max)(aabb1.min.y, aabb1.max.y);
		aabb1.min.z = (std::min)(aabb1.min.z, aabb1.max.z);
		aabb1.max.z = (std::max)(aabb1.min.z, aabb1.max.z);

		// aabb2のminとmaxの順序を保証
		aabb2.min.x = (std::min)(aabb2.min.x, aabb2.max.x);
		aabb2.max.x = (std::max)(aabb2.min.x, aabb2.max.x);
		aabb2.min.y = (std::min)(aabb2.min.y, aabb2.max.y);
		aabb2.max.y = (std::max)(aabb2.min.y, aabb2.max.y);
		aabb2.min.z = (std::min)(aabb2.min.z, aabb2.max.z);
		aabb2.max.z = (std::max)(aabb2.min.z, aabb2.max.z);


		// グリッド表示
		DrawGrid(viewProjectionMatrix, viewportMatrix);

		// 当たり判定して色決定
		uint32_t color = IsCollision(aabb1, aabb2) ? 0xFF0000FF : 0xFFFFFFFF;

		// AABB描画
		DrawAABB(aabb1, viewProjectionMatrix, viewportMatrix, color);
		DrawAABB(aabb2, viewProjectionMatrix, viewportMatrix, color);

		Novice::EndFrame();

		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) break;
	}

	Novice::Finalize();
	return 0;
}