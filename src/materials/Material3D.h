#pragma once
#ifndef Material3D_h__
#define Material3D_h__
#include <vector>
#include "MaterialBase.h"
#include <memory>
#include "Filter3d.h"

class Shader;
class Texture2D;
class Mesh;
class Filter3D;
typedef std::vector<std::shared_ptr<Filter3D>> filters_list;

class Material3D: public MaterialBase
{
private:
    filters_list _filters;
    std::vector<std::shared_ptr<Texture2D>> _textures;
public:
    Material3D(std::shared_ptr<Shader> shader);
    ~Material3D();
    void build() override;
	void bind(const RenderContext& ctx, const Mesh* mesh = nullptr) override;
	void unbind() override;
    void addFilter(std::shared_ptr<Filter3D> filter);
    const std::vector<std::shared_ptr<Texture2D>>& getTextures() const { return _textures; }
    void setTextures(const std::vector<std::shared_ptr<Texture2D>>& val) { _textures = val; }
    filters_list getFilters() const;
    std::shared_ptr<MaterialBase> clone() const override;
protected:

private:
    std::string getBlendingSign(const BlendMode& blendMode);
};

#endif // Material3D_h__
