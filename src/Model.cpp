#include "Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Device3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Pivot3D.h"
#include "Mesh.h"
#include <vector>

Model::Model():
    Pivot3D()
{

}

Model::~Model()
{

}



void Model::render(std::shared_ptr<MaterialBase> shader /*= nullptr*/)
{
	Pivot3D::render(shader);

}

