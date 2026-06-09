#include "Serialization.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <span>

#include "Pivot3D.h"
#include "render/MeshRenderer.h"
#include "render/Geometry.h"
#include "render/VertexLayout.h"
#include "render/VertexAttribute.h"
#include "materials/Material3D.h"
#include "materials/filters/Filter3d.h"
#include "materials/filters/FilterData.h"
#include "materials/filters/FilterFactory.h"
#include "lighting/PhongLightingModel.h"
#include "lighting/PBRLightingModel.h"
#include "lighting/UnlitLightingModel.h"
#include "utils/AssetPaths.h"
#include "behaviours/Behaviour.h"
#include "behaviours/BehaviourFactory.h"
#include "behaviours/reflection/Property.h"
#include "behaviours/reflection/References.h"

#include <functional>

using json = nlohmann::json;

namespace {

json Vec3ToJson(const glm::vec3& v) { return json::array({ v.x, v.y, v.z }); }
glm::vec3 Vec3FromJson(const json& j) { return glm::vec3(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>()); }

json Mat4ToJson(const glm::mat4& m) {
    json a = json::array();
    const float* p = glm::value_ptr(m);
    for (int i = 0; i < 16; ++i) a.push_back(p[i]);
    return a;
}
glm::mat4 Mat4FromJson(const json& j) {
    float v[16];
    for (int i = 0; i < 16; ++i) v[i] = j.at(i).get<float>();
    return glm::make_mat4(v);
}

json LayoutToJson(const VertexLayout& layout) {
    json lj;
    lj["stride"] = static_cast<uint64_t>(layout.GetStride());
    json attrs = json::array();
    for (const VertexAttribute& a : layout.GetAttributes()) {
        attrs.push_back({
            { "location", a.location },
            { "componentCount", a.componentCount },
            { "componentType", static_cast<unsigned int>(a.componentType) },
            { "normalize", a.normalize != GL_FALSE },
            { "offset", static_cast<uint64_t>(a.offset) },
            { "isInteger", a.isInteger },
        });
    }
    lj["attributes"] = attrs;
    return lj;
}

VertexLayout LayoutFromJson(const json& lj) {
    VertexLayout layout(lj.at("stride").get<size_t>());
    for (const json& a : lj.at("attributes")) {
        VertexAttribute attr{};
        attr.location = a.at("location").get<GLuint>();
        attr.componentCount = a.at("componentCount").get<GLint>();
        attr.componentType = static_cast<GLenum>(a.at("componentType").get<unsigned int>());
        attr.normalize = a.at("normalize").get<bool>() ? GL_TRUE : GL_FALSE;
        attr.offset = a.at("offset").get<size_t>();
        attr.isInteger = a.at("isInteger").get<bool>();
        layout.Add(attr);
    }
    return layout;
}

uint64_t AppendBlob(std::vector<std::byte>& bin, const void* data, size_t size) {
    const uint64_t offset = bin.size();
    const std::byte* p = static_cast<const std::byte*>(data);
    bin.insert(bin.end(), p, p + size);
    return offset;
}

std::unique_ptr<ILightingModel> CreateLighting(const std::string& name) {
    if (name == "PBR")   return std::make_unique<PBRLightingModel>();
    if (name == "Phong") return std::make_unique<PhongLightingModel>();
    return std::make_unique<UnlitLightingModel>();
}

int RegisterGeometry(SceneIO::SaveContext& ctx, const Geometry* g) {
    if (!g) return -1;
    if (auto it = ctx.geometryIds.find(g); it != ctx.geometryIds.end()) return it->second;

    const int id = static_cast<int>(ctx.geometries.size());
    ctx.geometryIds[g] = id;

    json gj;
    gj["layout"] = LayoutToJson(g->Layout());
    const auto& vd = g->VertexData();
    gj["vertexOffset"] = AppendBlob(ctx.bin, vd.data(), vd.size());
    gj["vertexSize"] = static_cast<uint64_t>(vd.size());
    const auto& idx = g->Indices();
    gj["indexOffset"] = AppendBlob(ctx.bin, idx.data(), idx.size() * sizeof(GLuint));
    gj["indexCount"] = static_cast<uint64_t>(idx.size());

    json secondary = json::array();
    for (const auto& s : g->SecondaryBuffers()) {
        json e;
        e["layout"] = LayoutToJson(s.layout);
        e["offset"] = AppendBlob(ctx.bin, s.data.data(), s.data.size());
        e["size"] = static_cast<uint64_t>(s.data.size());
        secondary.push_back(e);
    }
    gj["secondary"] = secondary;

    ctx.geometries.push_back(std::move(gj));
    return id;
}

int RegisterMaterial(SceneIO::SaveContext& ctx, const MaterialBase* base) {
    const Material3D* mat = dynamic_cast<const Material3D*>(base);
    if (!mat) return -1;
    if (auto it = ctx.materialIds.find(mat); it != ctx.materialIds.end()) return it->second;

    const int id = static_cast<int>(ctx.materials.size());
    ctx.materialIds[mat] = id;

    json mj;
    mj["vertexShader"] = mat->GetVertexShaderPath();
    mj["fragmentShader"] = mat->GetFragmentShaderPath();
    mj["lighting"] = mat->GetLightingTypeName();
    mj["roughnessScale"] = mat->GetRoughnessScale();
    mj["genVersion"] = Material3D::ShaderGenVersion();
    mj["compiled"] = {
        { "vertex", mat->GetCompiledVertexSource() },
        { "fragment", mat->GetCompiledFragmentSource() },
    };

    json filters = json::array();
    for (const auto& f : mat->GetFilters()) {
        const FilterData d = f->Serialize();
        filters.push_back({
            { "type", d.type },
            { "slot", static_cast<int>(d.slot) },
            { "blend", static_cast<int>(d.blend) },
            { "texturePath", d.texturePath },
            { "color", d.color },
        });
    }
    mj["filters"] = filters;

    ctx.materials.push_back(std::move(mj));
    return id;
}

std::shared_ptr<Geometry> GeometryFromJson(const json& gj, const std::vector<std::byte>& bin) {
    VertexLayout layout = LayoutFromJson(gj.at("layout"));

    const auto vertexOffset = gj.at("vertexOffset").get<uint64_t>();
    const auto vertexSize   = gj.at("vertexSize").get<uint64_t>();
    const auto indexOffset  = gj.at("indexOffset").get<uint64_t>();
    const auto indexCount   = gj.at("indexCount").get<uint64_t>();

    std::span<const std::byte> vertexSpan(bin.data() + vertexOffset, vertexSize);
    std::vector<GLuint> indices(indexCount);
    if (indexCount > 0) {
        std::memcpy(indices.data(), bin.data() + indexOffset, indexCount * sizeof(GLuint));
    }

    auto geometry = std::make_shared<Geometry>(layout, vertexSpan, std::span<const GLuint>(indices));

    if (gj.contains("secondary")) {
        for (const json& e : gj.at("secondary")) {
            VertexLayout sLayout = LayoutFromJson(e.at("layout"));
            const auto off = e.at("offset").get<uint64_t>();
            const auto size = e.at("size").get<uint64_t>();
            geometry->AddSecondaryBuffer(sLayout, std::span<const std::byte>(bin.data() + off, size));
        }
    }
    return geometry;
}

std::shared_ptr<Material3D> MaterialFromJson(const json& mj) {
    auto mat = std::make_shared<Material3D>(
        mj.value("vertexShader", AssetPaths::Resolve("shaders/shader.vsh")),
        mj.value("fragmentShader", AssetPaths::Resolve("shaders/defaultColorLight.fsh")));

    mat->SetLightingModel(CreateLighting(mj.value("lighting", "Unlit")));
    mat->SetRoughnessScale(mj.value("roughnessScale", 1.0f));

    if (mj.contains("filters")) {
        for (const json& fj : mj.at("filters")) {
            FilterData d;
            d.type = fj.value("type", "");
            d.slot = static_cast<Filter3D::FilterSlot>(fj.value("slot", 0));
            d.blend = static_cast<Filter3D::BlendMode>(fj.value("blend", 0));
            d.texturePath = fj.value("texturePath", "");
            d.color = fj.value("color", 0xFFFFFFFFu);
            if (auto filter = FilterFactory::Create(d)) {
                mat->AddFilter(filter);
            }
        }
    }

    const bool hasCompiled = mj.contains("compiled")
        && mj.value("genVersion", -1) == Material3D::ShaderGenVersion();
    if (hasCompiled) {
        const json& c = mj.at("compiled");
        mat->BuildCompiled(c.value("vertex", std::string()), c.value("fragment", std::string()));
    } else {
        mat->Build();
    }
    return mat;
}

// ───────────────────────── Behaviours (компоненты) ─────────────────────────
// Значения свойств (де)сериализуются обобщённо по дескрипторам Behaviour::GetProperties().

json ScalarToJson(PropertyType type, void* field) {
    switch (type) {
    case PropertyType::Bool:   return *static_cast<bool*>(field);
    case PropertyType::Int:    return *static_cast<int*>(field);
    case PropertyType::Float:  return *static_cast<float*>(field);
    case PropertyType::String: return *static_cast<std::string*>(field);
    case PropertyType::Vec3:
    case PropertyType::Color:  return Vec3ToJson(*static_cast<glm::vec3*>(field));
    case PropertyType::NodeRef: {
        const auto* r = static_cast<NodeRef*>(field);
        return json{ { "node", r->id } };
    }
    case PropertyType::BehRef: {
        const auto* r = static_cast<BehRef*>(field);
        return json{ { "node", r->nodeId }, { "type", r->type } };
    }
    default: return nullptr;
    }
}

void ScalarFromJson(PropertyType type, void* field, const json& j) {
    switch (type) {
    case PropertyType::Bool:   *static_cast<bool*>(field) = j.get<bool>(); break;
    case PropertyType::Int:    *static_cast<int*>(field) = j.get<int>(); break;
    case PropertyType::Float:  *static_cast<float*>(field) = j.get<float>(); break;
    case PropertyType::String: *static_cast<std::string*>(field) = j.get<std::string>(); break;
    case PropertyType::Vec3:
    case PropertyType::Color:  *static_cast<glm::vec3*>(field) = Vec3FromJson(j); break;
    case PropertyType::NodeRef: {
        auto* r = static_cast<NodeRef*>(field);
        r->id = j.value("node", 0u);
        r->cached.reset();
        break;
    }
    case PropertyType::BehRef: {
        auto* r = static_cast<BehRef*>(field);
        r->nodeId = j.value("node", 0u);
        r->type = j.value("type", std::string());
        r->cached = nullptr;
        break;
    }
    default: break;
    }
}

json PropertyToJson(const Property& p, void* field) {
    if (p.type == PropertyType::List) {
        json arr = json::array();
        const std::size_t n = p.listSize(field);
        for (std::size_t i = 0; i < n; ++i) {
            arr.push_back(ScalarToJson(p.elementType, p.listAt(field, i)));
        }
        return arr;
    }
    return ScalarToJson(p.type, field);
}

void PropertyFromJson(const Property& p, void* field, const json& j) {
    if (p.type == PropertyType::List) {
        if (p.listClear) p.listClear(field);
        if (j.is_array()) {
            for (const json& elem : j) {
                p.listAdd(field);
                const std::size_t last = p.listSize(field) - 1;
                ScalarFromJson(p.elementType, p.listAt(field, last), elem);
            }
        }
        return;
    }
    ScalarFromJson(p.type, field, j);
}

json BehaviourToJson(const Behaviour& behaviour) {
    json bj;
    bj["type"] = behaviour.GetTypeName();
    bj["enabled"] = behaviour.IsEnabled();
    json params = json::object();
    for (const Property& p : behaviour.GetProperties()) {
        void* field = p.resolve(const_cast<Behaviour*>(&behaviour));
        params[p.name] = PropertyToJson(p, field);
    }
    bj["params"] = params;
    return bj;
}

void ApplyBehaviourParams(Behaviour& behaviour, const json& params) {
    for (const Property& p : behaviour.GetProperties()) {
        if (params.contains(p.name)) {
            PropertyFromJson(p, p.resolve(&behaviour), params.at(p.name));
        }
    }
}

void ResolveScalarRef(PropertyType type, void* field, Pivot3D& root) {
    if (type == PropertyType::NodeRef) {
        auto* r = static_cast<NodeRef*>(field);
        if (r->id != 0) {
            r->cached = root.GetChildById(r->id);
        }
    } else if (type == PropertyType::BehRef) {
        auto* r = static_cast<BehRef*>(field);
        if (r->nodeId != 0) {
            if (auto node = root.GetChildById(r->nodeId)) {
                r->cached = r->type.empty()
                    ? (node->GetBehaviours().empty() ? nullptr : node->GetBehaviours().front().get())
                    : node->FindBehaviourByType(r->type);
            }
        }
    }
}

} // namespace

namespace SceneIO {

json NodeToJson(SaveContext& ctx, const Pivot3D& node) {
    json nj;
    nj["name"] = node.GetName();
    nj["id"] = node.GetId(); // нужно для резолва ссылок NodeRef/BehRef после загрузки
    nj["transform"] = {
        { "position", Vec3ToJson(*node.GetPosition()) },
        { "rotation", Vec3ToJson(*node.GetRotation()) },
        { "scale",    Vec3ToJson(*node.GetScale()) },
    };
    nj["castShadows"] = node.GetCastShadows();
    nj["receiveShadows"] = node.GetReceiveShadows();

    if (const MeshRenderer* renderer = node.GetRenderer()) {
        nj["nodeMatrix"] = Mat4ToJson(renderer->GetNodeMatrix());
        nj["geometry"] = RegisterGeometry(ctx, renderer->GetGeometry().get());
        nj["material"] = RegisterMaterial(ctx, renderer->GetMaterial().get());
    } else {
        nj["geometry"] = -1;
        nj["material"] = -1;
    }

    json behaviours = json::array();
    for (const auto& behaviour : node.GetBehaviours()) {
        if (behaviour) {
            behaviours.push_back(BehaviourToJson(*behaviour));
        }
    }
    nj["behaviours"] = behaviours;

    json children = json::array();
    for (const auto& child : const_cast<Pivot3D&>(node).Children()) {
        children.push_back(NodeToJson(ctx, *child));
    }
    nj["children"] = children;
    return nj;
}

std::vector<std::shared_ptr<Geometry>> BuildGeometries(const json& geometries, const std::vector<std::byte>& bin) {
    std::vector<std::shared_ptr<Geometry>> out;
    for (const json& gj : geometries) {
        out.push_back(GeometryFromJson(gj, bin));
    }
    return out;
}

std::vector<std::shared_ptr<Material3D>> BuildMaterials(const json& materials) {
    std::vector<std::shared_ptr<Material3D>> out;
    for (const json& mj : materials) {
        out.push_back(MaterialFromJson(mj));
    }
    return out;
}

std::shared_ptr<Pivot3D> NodeFromJson(const json& nj,
                                      const std::vector<std::shared_ptr<Geometry>>& geometries,
                                      const std::vector<std::shared_ptr<Material3D>>& materials) {
    const int g = nj.value("geometry", -1);
    const int m = nj.value("material", -1);

    std::shared_ptr<Pivot3D> node;
    if (g >= 0 && m >= 0 && g < static_cast<int>(geometries.size()) && m < static_cast<int>(materials.size())) {
        node = MeshRenderer::MakeNode(geometries[g], materials[m]);
        if (nj.contains("nodeMatrix")) {
            node->GetRenderer()->SetNodeMatrix(Mat4FromJson(nj.at("nodeMatrix")));
        }
    } else {
        node = std::make_shared<Pivot3D>();
    }

    node->SetName(nj.value("name", std::string("Node")));
    const json& t = nj.at("transform");
    const glm::vec3 p = Vec3FromJson(t.at("position"));
    const glm::vec3 r = Vec3FromJson(t.at("rotation"));
    const glm::vec3 s = Vec3FromJson(t.at("scale"));
    node->SetPosition(p.x, p.y, p.z);
    node->SetRotation(r.x, r.y, r.z);
    node->SetScale(s.x, s.y, s.z);
    node->SetCastShadows(nj.value("castShadows", true));
    node->SetReceiveShadows(nj.value("receiveShadows", true));
    // Восстанавливаем id, чтобы ссылки NodeRef/BehRef резолвились (см. ResolveReferences).
    node->SetId(nj.value("id", node->GetId()));

    if (nj.contains("behaviours")) {
        for (const json& bj : nj.at("behaviours")) {
            const std::string type = bj.value("type", std::string());
            auto behaviour = BehaviourFactory::Create(type);
            if (!behaviour) {
                continue; // незарегистрированный тип (Camera/Animator/...) — пропускаем
            }
            behaviour->SetEnabled(bj.value("enabled", true));
            Behaviour* raw = node->AddBehaviour(std::move(behaviour));
            if (raw && bj.contains("params")) {
                ApplyBehaviourParams(*raw, bj.at("params"));
            }
        }
    }

    if (nj.contains("children")) {
        for (const json& cj : nj.at("children")) {
            node->AddChild(NodeFromJson(cj, geometries, materials));
        }
    }
    return node;
}

void ResolveReferences(Pivot3D& root) {
    std::function<void(Pivot3D&)> visit = [&](Pivot3D& node) {
        for (const auto& behaviour : node.GetBehaviours()) {
            if (!behaviour) continue;
            for (const Property& p : behaviour->GetProperties()) {
                void* field = p.resolve(behaviour.get());
                if (p.type == PropertyType::List) {
                    const std::size_t n = p.listSize(field);
                    for (std::size_t i = 0; i < n; ++i) {
                        ResolveScalarRef(p.elementType, p.listAt(field, i), root);
                    }
                } else {
                    ResolveScalarRef(p.type, field, root);
                }
            }
        }
        for (const auto& child : node.Children()) {
            visit(*child);
        }
    };
    visit(root);
}

} // namespace SceneIO
