#include <Novice.h>
#include <cmath>
#include <imgui.h>

const char kWindowTitle[] = "LE2C_21_チハラ_シゴウ";

struct Vector3 {
	float x, y, z;
};

struct Matrix4x4 {
	float m[4][4];
};

struct Plane {

	Vector3 normal; //!<法線
	float distance; //!<距離
};

struct Segment {
	Vector3 origin;  //!< 始点
	Vector3 diff;    //!< 終点への差分ベクトル
};

struct Triangle {
	Vector3 vertices[3];
};

float Dot(const Vector3& a, const Vector3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 Cross(const Vector3& a, const Vector3& b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

// 衝突判定関数（true：交差する）
bool IsCollision(const Segment& segment, const Plane& plane) {
	float dotA = segment.origin.x * plane.normal.x + segment.origin.y * plane.normal.y + segment.origin.z * plane.normal.z - plane.distance;
	Vector3 end = {
		segment.origin.x + segment.diff.x,
		segment.origin.y + segment.diff.y,
		segment.origin.z + segment.diff.z
	};
	float dotB = end.x * plane.normal.x + end.y * plane.normal.y + end.z * plane.normal.z - plane.distance;

	return (dotA * dotB <= 0.0f); // 片方が裏なら交差
}


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

Vector3 cameraTranslate = { 0.0f, 2.0f, -7.0f };
Vector3 cameraRotate = { 0.0f, 0.0f, 0.0f };

Vector3 Perpendicular(const Vector3& vector) {
	if (vector.x != 0.0f || vector.y != 0.0f) {

		return { -vector.y, vector.x, 0.0f };

	}
	return { 0.0f, -vector.z, vector.y };
}


void DrawPlane(const Plane& plane, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
	// 平面の中心座標
	Vector3 center = {
		plane.normal.x * plane.distance,
		plane.normal.y * plane.distance,
		plane.normal.z * plane.distance
	};

	// 法線と垂直なベクトル2つ
	Vector3 tangent = Normalize(Perpendicular(plane.normal));
	Vector3 bitangent = Normalize(Cross(plane.normal, tangent));

	float planeSize = 2.0f; // 描画サイズ半径

	// 平面の4頂点
	Vector3 localPoints[4] = {
		{ -planeSize,  0.0f, -planeSize },
		{  planeSize,  0.0f, -planeSize },
		{  planeSize,  0.0f,  planeSize },
		{ -planeSize,  0.0f,  planeSize }
	};

	Vector3 worldPoints[4];
	for (int i = 0; i < 4; ++i) {
		// tangentsとbitangentsの組み合わせでXY平面に矩形生成
		Vector3 offset = {
			tangent.x * localPoints[i].x + bitangent.x * localPoints[i].z,
			tangent.y * localPoints[i].x + bitangent.y * localPoints[i].z,
			tangent.z * localPoints[i].x + bitangent.z * localPoints[i].z
		};
		Vector3 worldPos = {
			center.x + offset.x,
			center.y + offset.y,
			center.z + offset.z
		};
		worldPoints[i] = Transform(Transform(worldPos, viewProjectionMatrix), viewportMatrix);
	}

	for (int i = 0; i < 4; ++i) {
		int next = (i + 1) % 4;
		Novice::DrawLine((int)worldPoints[i].x, (int)worldPoints[i].y, (int)worldPoints[next].x, (int)worldPoints[next].y, color);
	}
}


void DrawSegment(const Segment& segment, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
	Vector3 start = Transform(Transform(segment.origin, viewProjectionMatrix), viewportMatrix);
	Vector3 end = {
		segment.origin.x + segment.diff.x,
		segment.origin.y + segment.diff.y,
		segment.origin.z + segment.diff.z
	};
	Vector3 endTransformed = Transform(Transform(end, viewProjectionMatrix), viewportMatrix);

	Novice::DrawLine((int)start.x, (int)start.y, (int)endTransformed.x, (int)endTransformed.y, color);
}

void DrawTriangle(const Triangle& triangle, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
	Vector3 screenV[3];
	for (int i = 0; i < 3; ++i) {
		screenV[i] = Transform(Transform(triangle.vertices[i], viewProjectionMatrix), viewportMatrix);
	}
	Novice::DrawTriangle(
		(int)screenV[0].x, (int)screenV[0].y,
		(int)screenV[1].x, (int)screenV[1].y,
		(int)screenV[2].x, (int)screenV[2].y,
		color, kFillModeWireFrame
	);
}

bool IsCollision(const Triangle& triangle, const Segment& segment) {
	// 三角形の法線を求める
	Vector3 v01 = {
		triangle.vertices[1].x - triangle.vertices[0].x,
		triangle.vertices[1].y - triangle.vertices[0].y,
		triangle.vertices[1].z - triangle.vertices[0].z
	};
	Vector3 v02 = {
		triangle.vertices[2].x - triangle.vertices[0].x,
		triangle.vertices[2].y - triangle.vertices[0].y,
		triangle.vertices[2].z - triangle.vertices[0].z
	};

	Vector3 normal = Normalize(Cross(v01, v02));

	// 三角形を含む平面
	float d = triangle.vertices[0].x * normal.x + triangle.vertices[0].y * normal.y + triangle.vertices[0].z * normal.z;

	// 始点と終点
	Vector3 p0 = segment.origin;
	Vector3 p1 = {
		segment.origin.x + segment.diff.x,
		segment.origin.y + segment.diff.y,
		segment.origin.z + segment.diff.z
	};

	// 始点終点の平面からの距離
	float dot0 = p0.x * normal.x + p0.y * normal.y + p0.z * normal.z - d;
	float dot1 = p1.x * normal.x + p1.y * normal.y + p1.z * normal.z - d;

	if (dot0 * dot1 > 0.0f) return false; // 平面と交差していない

	// 衝突点を線形補間で求める
	float t = dot0 / (dot0 - dot1);
	Vector3 p = {
		p0.x + (p1.x - p0.x) * t,
		p0.y + (p1.y - p0.y) * t,
		p0.z + (p1.z - p0.z) * t,
	};

	// バリツェントリック法で三角形内にあるかをチェック
	Vector3 v0 = {
		triangle.vertices[1].x - triangle.vertices[0].x,
		triangle.vertices[1].y - triangle.vertices[0].y,
		triangle.vertices[1].z - triangle.vertices[0].z,
	};
	Vector3 v1 = {
		triangle.vertices[2].x - triangle.vertices[0].x,
		triangle.vertices[2].y - triangle.vertices[0].y,
		triangle.vertices[2].z - triangle.vertices[0].z,
	};
	Vector3 v2 = {
		p.x - triangle.vertices[0].x,
		p.y - triangle.vertices[0].y,
		p.z - triangle.vertices[0].z,
	};

	float d00 = Dot(v0, v0);
	float d01 = Dot(v0, v1);
	float d11 = Dot(v1, v1);
	float d20 = Dot(v2, v0);
	float d21 = Dot(v2, v1);

	float denom = d00 * d11 - d01 * d01;
	if (denom == 0.0f) return false;

	float u = (d11 * d20 - d01 * d21) / denom;
	float v = (d00 * d21 - d01 * d20) / denom;

	return (u >= 0.0f && v >= 0.0f && u + v <= 1.0f);
}



// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, 1280, 720);

	// キー入力結果を受け取る箱
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	static Plane plane = { {0.0f, 1.0f, 0.0f}, 1.0f };
	static Segment segment = { {0.0f, 1.0f, -1.0f}, {0.0f, -2.0f, 2.0f} };



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

		plane.normal = Normalize(plane.normal);
		Matrix4x4 rotateMatrix = MakeRotateMatrix(cameraRotate);
		Vector3 forward = Transform({ 0, 0, 1 }, rotateMatrix);
		Vector3 target = {
			cameraTranslate.x + forward.x,
			cameraTranslate.y + forward.y,
			cameraTranslate.z + forward.z
		};
		Vector3 up = Transform({ 0, 1, 0 }, rotateMatrix);

		Matrix4x4 viewMatrix = MakeViewMatrix(cameraTranslate, target, up);

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

		static Triangle triangle = {
			{{-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}
		};


		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		// ImGui 操作パネル
		ImGui::Begin("Window");
		ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
		ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
		ImGui::DragFloat3("Plane.Normal", &plane.normal.x, 0.01f);
		ImGui::DragFloat("Plane.Distance", &plane.distance, 0.01f);
		ImGui::DragFloat3("Segment Origin", &segment.origin.x, 0.01f);
		ImGui::DragFloat3("Segment Diff", &segment.diff.x, 0.01f);
		ImGui::DragFloat3("Triangle V0", &triangle.vertices[0].x, 0.01f);
		ImGui::DragFloat3("Triangle V1", &triangle.vertices[1].x, 0.01f);
		ImGui::DragFloat3("Triangle V2", &triangle.vertices[2].x, 0.01f);


		ImGui::End();

		uint32_t segColor = IsCollision(segment, plane) ? 0xFF0000FF : 0xFFFFFFFF; // 赤 or 白
		DrawSegment(segment, viewProjectionMatrix, viewportMatrix, segColor);
		DrawPlane(plane, viewProjectionMatrix, viewportMatrix, WHITE);
		DrawGrid(viewProjectionMatrix, viewportMatrix);
		bool hit = IsCollision(triangle, segment);
		DrawTriangle(triangle, viewProjectionMatrix, viewportMatrix, 0x00FF00FF); // 緑の三角形
		DrawSegment(segment, viewProjectionMatrix, viewportMatrix, hit ? 0xFF0000FF : 0xFFFFFFFF); // 赤 or 白

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