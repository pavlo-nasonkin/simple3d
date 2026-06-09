#include "Renderer.h"

#include <algorithm>
#include <limits>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Scene3D.h"
#include "Pivot3D.h"
#include "render/RenderContext.h"
#include "render/ShadowMap.h"
#include "render/Skybox.h"
#include "render/MeshRenderer.h"
#include "render/Geometry.h"
#include "render/DrawItem.h"
#include "render/Frustum.h"
#include "materials/DepthMaterial.h"
#include "materials/MaterialBase.h"

namespace {

// World-AABB = bbox 8 трансформированных углов локального AABB.
void WorldAABB(const glm::mat4& m, const glm::vec3& mn, const glm::vec3& mx,
               glm::vec3& outMin, glm::vec3& outMax)
{
    outMin = glm::vec3(std::numeric_limits<float>::max());
    outMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (int i = 0; i < 8; ++i) {
        const glm::vec3 corner((i & 1) ? mx.x : mn.x,
                               (i & 2) ? mx.y : mn.y,
                               (i & 4) ? mx.z : mn.z);
        const glm::vec3 w = glm::vec3(m * glm::vec4(corner, 1.0f));
        outMin = glm::min(outMin, w);
        outMax = glm::max(outMax, w);
    }
}

// Сбор очереди → frustum culling → сортировка по материалу/геометрии → отрисовка.
void DrawQueue(Scene3D& scene, const RenderContext& ctx, MaterialBase* overrideMaterial)
{
    std::vector<DrawItem> items;
    scene.CollectDrawItems(ctx.model, ctx.shadowPass, items);

    const Frustum frustum = Frustum::FromViewProj(ctx.projection * ctx.view);

    std::vector<DrawItem> visible;
    visible.reserve(items.size());
    for (const DrawItem& item : items) {
        const Geometry* geo = item.renderer->GetGeometry().get();
        if (geo && geo->HasBounds()) {
            const glm::mat4 cullWorld = item.world * item.renderer->GetNodeMatrix();
            glm::vec3 mn, mx;
            WorldAABB(cullWorld, geo->AABBMin(), geo->AABBMax(), mn, mx);
            if (!frustum.IntersectsAABB(mn, mx)) {
                continue;
            }
        }
        visible.push_back(item);
    }

    // Группируем по материалу, затем по геометрии — меньше переключений состояния
    // и задел под инстансинг (одинаковые material+geometry идут подряд).
    std::sort(visible.begin(), visible.end(), [overrideMaterial](const DrawItem& a, const DrawItem& b) {
        const void* ma = overrideMaterial ? static_cast<const void*>(overrideMaterial)
                                          : static_cast<const void*>(a.renderer->GetMaterial().get());
        const void* mb = overrideMaterial ? static_cast<const void*>(overrideMaterial)
                                          : static_cast<const void*>(b.renderer->GetMaterial().get());
        if (ma != mb) {
            return ma < mb;
        }
        return a.renderer->GetGeometry().get() < b.renderer->GetGeometry().get();
    });

    RenderContext drawCtx = ctx;
    for (const DrawItem& item : visible) {
        drawCtx.model = item.world;
        item.renderer->Draw(drawCtx, item.owner, overrideMaterial);
    }
}

} // namespace

void Renderer::RenderScene(Scene3D& scene, const RenderContext& ctx, MaterialBase* overrideMaterial)
{
    scene.RefreshActiveLights();

    RenderContext mainCtx = ctx;

    ShadowMap* shadowMap = scene.GetShadowMap();
    MaterialBase* depthMaterial = scene.GetDepthMaterial();

    // Shadow-pass: глубина сцены из точки зрения солнца (только в обычном цветовом проходе).
    if (shadowMap && depthMaterial && !overrideMaterial) {
        const glm::mat4 lightSpace = ShadowMap::ComputeLightSpaceMatrix(
            scene.GetEffectiveDirLightDirection(), scene.GetShadowCenter(), scene.GetShadowRadius());

        RenderContext shadowCtx = ctx;
        shadowCtx.view = glm::mat4(1.0f);     // view*projection = lightSpace * I
        shadowCtx.projection = lightSpace;
        shadowCtx.shadowPass = true;

        shadowMap->Begin();
        DrawQueue(scene, shadowCtx, depthMaterial); // cull по ortho-боксу света
        shadowMap->End();

        mainCtx.lightSpaceMatrix = lightSpace;
        mainCtx.shadowMap = shadowMap->DepthTexture();
        mainCtx.hasShadows = true;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawQueue(scene, mainCtx, overrideMaterial);

    // Skybox — последним и только в цветовом проходе (не в id-pass'е пикинга).
    if (Skybox* skybox = scene.GetSkybox(); skybox && !overrideMaterial) {
        skybox->Render(mainCtx.view, mainCtx.projection);
    }
}
