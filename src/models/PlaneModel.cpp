#include "PlaneModel.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "Engine.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include "materials/Material3D.h"
#include "materials/filters/ColorFilter.h"
#include "materials/filters/TextureMapFilter.h"
#include "materials/filters/NormalMapFilter.h"
#include "lighting/PBRLightingModel.h"
#include "render/VertexLayoutPresets.h"
#include "resources/Texture2D.h"
#include "resources/TextureManager.h"

namespace {
	struct PlaneSlot {
		Filter3D::FilterSlot slot;
		std::string typeName;
		Filter3D::BlendMode blend;
		bool normalMap;
		std::vector<std::string> suffixes; // по приоритету, после последнего '_'
	};

	const std::vector<PlaneSlot> kPlaneSlots = {
		{ Filter3D::FilterSlot::BaseColor, "texture_diffuse",   Filter3D::BlendMode::MULTIPLY, false, { "BaseColor", "Albedo", "Diffuse" } },
		{ Filter3D::FilterSlot::Normal,    "texture_normal",    Filter3D::BlendMode::NORMAL,   true,  { "Normal" } },
		{ Filter3D::FilterSlot::Roughness, "texture_roughness", Filter3D::BlendMode::NORMAL,   false, { "Roughness" } },
		{ Filter3D::FilterSlot::AO,        "texture_ao",        Filter3D::BlendMode::NORMAL,   false, { "AO", "AmbientOcclusion", "Occlusion" } },
		{ Filter3D::FilterSlot::Metallic,  "texture_metalness", Filter3D::BlendMode::NORMAL,   false, { "Metalness", "Metallic" } },
	};

	std::string ToLower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(),
		               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return s;
	}

	bool IsImageExtension(const std::string& ext) {
		const std::string e = ToLower(ext);
		return e == ".jpg" || e == ".jpeg" || e == ".png" || e == ".tga" || e == ".bmp";
	}

	// Ищет в каталоге файл, у которого stem заканчивается на "_<suffix>" (по приоритету).
	std::string FindByConvention(const std::string& directory, const std::vector<std::string>& suffixes) {
		namespace fs = std::filesystem;
		std::error_code ec;
		if (directory.empty() || !fs::is_directory(directory, ec)) {
			return {};
		}
		for (const auto& suffix : suffixes) {
			const std::string token = "_" + ToLower(suffix);
			for (const auto& entry : fs::directory_iterator(directory, ec)) {
				if (ec) break;
				if (!entry.is_regular_file(ec)) continue;
				const auto& path = entry.path();
				if (!IsImageExtension(path.extension().string())) continue;
				const std::string stem = ToLower(path.stem().string());
				if (stem.size() >= token.size() &&
				    stem.compare(stem.size() - token.size(), token.size(), token) == 0) {
					return path.filename().string();
				}
			}
		}
		return {};
	}
}

PlaneModel::PlaneModel()
{
}

void PlaneModel::Init()
{
    AddChild(ProcessMesh());
}

std::shared_ptr<Mesh> PlaneModel::ProcessMesh()
{
	auto mat = std::make_shared<Material3D>("../assets/shaders/shader.vsh",
														  "../assets/shaders/defaultColorLight.fsh");
	mat->SetRoughnessScale(_roughnessScale);

	if (_materialDir.empty()) {
		// дефолт: плоский цвет
		_colorFilter = std::make_shared<ColorFilter>();
		_colorFilter->SetColor(_color);
		_colorFilter->SetBlendMode(Filter3D::BlendMode::MULTIPLY);
		mat->AddFilter(_colorFilter);
	} else {
		// PBR-материал из папки по соглашению Megascans
		mat->SetLightingModel(std::make_unique<PBRLightingModel>());
		auto& texManager = Engine::GetInstance().GetTextureManager();
		for (const auto& cfg : kPlaneSlots) {
			const std::string filename = FindByConvention(_materialDir, cfg.suffixes);
			if (filename.empty()) continue;
			auto tex = texManager->getTexture(filename, cfg.typeName, _materialDir);
			if (!tex || tex->id == 0) continue;

			std::shared_ptr<Filter3D> filter = cfg.normalMap
				? std::static_pointer_cast<Filter3D>(std::make_shared<NormalMapFilter>(tex))
				: std::static_pointer_cast<Filter3D>(std::make_shared<TextureMapFilter>(tex));
			filter->SetSlot(cfg.slot);
			filter->SetBlendMode(cfg.blend);
			mat->AddFilter(filter);
		}
	}

	mat->Build();

	// per-instance копия вершин с UV, домноженными на _tiling (чтобы текстура не растягивалась)
	std::vector<VertexTypes::Vertex> verts = planeVertices;
	if (_tiling != 1.0f) {
		for (auto& v : verts) {
			v.TexCoords *= _tiling;
		}
	}

	auto mesh = std::make_shared<Mesh>(mat);
	mesh->SetupMesh(VertexLayouts::Standard(), std::as_bytes(std::span(verts)), std::span(planeIndices));
	mesh->SetName(std::string("Plane") + std::to_string(mesh->GetId()));
	return mesh;
}

void PlaneModel::SetColor(unsigned int color) {
	_color = color;
	if (_colorFilter) {
		_colorFilter->SetColor(color);
	}
}

// CCW при взгляде сверху (+Y), front-face видна сверху.
std::vector<GLuint> PlaneModel::planeIndices = {
	0, 3, 2,
	0, 2, 1,
};

std::vector<VertexTypes::Vertex> PlaneModel::planeVertices = {
	// +Y face: N = (0, 1, 0), T = (1, 0, 0)
	{ glm::vec3(-0.5f, 0.0f, -0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(0.0f, 0.0f),	glm::vec3(1.0f, 0.0f, 0.0f) },
	{ glm::vec3( 0.5f, 0.0f, -0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(1.0f, 0.0f),	glm::vec3(1.0f, 0.0f, 0.0f) },
	{ glm::vec3( 0.5f, 0.0f,  0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(1.0f, 1.0f),	glm::vec3(1.0f, 0.0f, 0.0f) },
	{ glm::vec3(-0.5f, 0.0f,  0.5f),	glm::vec3(0.0f, 1.0f, 0.0f),	glm::vec2(0.0f, 1.0f),	glm::vec3(1.0f, 0.0f, 0.0f) },
};
