#include "groundwork.h"


// push model to the vector in camera
//int Collision_Detection::push_model(Model * model)
//{
//	ObjectSphereVec objSphV = objectSphereVecs[model->index];
//	ObjectSphere newObjSph;
//
//	XMStoreFloat3(&newObjSph.center, objSphV.center);
//	newObjSph.radius = XMVectorGetX(objSphV.radius);
//	p_objectSpheres->push_back(newObjSph);
//	
//	//return index
//	return p_objectSpheres->size() - 1;
//
//}

//// store the bounding sphere in the array with the unchanged bounding spheres
//int Collision_Detection::store_model(ObjectSphereVec & objSphV)
//{
//	objectSphereVecs.push_back(objSphV);
//	
//	//return index
//	return objectSphereVecs.size() - 1;
//}

Model::Model()
{
	vertex_buffer = NULL;
	vertex_count = 0;
	//index = 0;
	g_pTexture = NULL;
	boundary = NULL;
}

Model::Model(const Model & other)
{
	this->vertex_buffer = other.vertex_buffer;
	this->vertex_count = other.vertex_count;

	const Sphere_Boundary *pSB = dynamic_cast<const Sphere_Boundary*>(other.boundary);
	const Box_Boundary *pBB = dynamic_cast<const Box_Boundary*>(other.boundary);
	const Coord_Boundary *pCB = dynamic_cast<const Coord_Boundary*>(other.boundary);

	if (pSB)
		this->boundary = new Sphere_Boundary(pSB->center, pSB->radius);
	else if (pBB)
		this->boundary = new Box_Boundary(pBB->max_x, pBB->max_y, pBB->max_z, pBB->min_x, pBB->min_y, pBB->min_z);
	else if (pCB)
		this->boundary = new Coord_Boundary();
}

Model & Model::operator=(const Model & other)
{
	this->vertex_buffer = other.vertex_buffer;
	this->vertex_count = other.vertex_count;
	this->boundary = other.boundary;

	return *this;
}

Model::~Model()
{
	delete this->boundary;
}

//loads a model from 3ds file into vertexbuffer and calculates and stores bounding sphere
void Model::load_Model(char * filename, ID3D11Device * device, BOUNDARY_TYPE b, bool g)
{
	//initialize members
	vertex_buffer = NULL;
	vertex_count = 0;

	//index = 0;

	//load 3ds file
	Load3DS(filename, device, &vertex_buffer, &vertex_count, b, this, g);

	//add to permanent storage vector of collision detection
	//c_d->push_model(this);
}

//update the models bounding sphere, with the worldmatrix M
//scaling matrix must be passed seperately at this point to correctly scale the radius of the sphere..
//call this before every draw call
//void Model::update_objectSphere(Collision_Detection * c_d, XMMATRIX & M, XMMATRIX & S)
//{
//	//apply matrix m to object sphere
//	XMFLOAT3 center;
//	float radius;
//
//	//transform vector to center and store in float3
//	XMStoreFloat3(&center, XMVector3Transform(c_d->objectSphereVecs[index].center, M));
//	//scale radius vector, calculate length and stor in a float
//	radius = XMVectorGetX(XMVector3Length((XMVector3Transform(c_d->objectSphereVecs[index].radius, S))));
//
//	//update the values in the bounding sphere vector with changing values
//	c_d->p_objectSpheres->at(index).center = center;
//	c_d->p_objectSpheres->at(index).radius = radius;
//}

Sphere_Boundary::Sphere_Boundary(XMFLOAT3 center, float radius) : center{ center }, radius{ radius }
{
}

/*Sphere_Boundary::Sphere_Boundary(const Sphere_Boundary & other)
{
this->center = other.center;
this->radius = other.radius;
}

Boundary & Sphere_Boundary::operator=(const Boundary & other)
{
const Sphere_Boundary *pSB = dynamic_cast<const Sphere_Boundary*>(&other);
this->center = pSB->center;
this->radius = pSB->radius;

return *this;
}



Sphere_Boundary& Sphere_Boundary::operator=(const Sphere_Boundary & other)
{
this->center = other.center;
this->radius = other.radius;

return *this;
}
*/


float Sphere_Boundary::calculate_distance(XMFLOAT3 pos)
{
	float d_center = sqrtf((center.x - pos.x)*(center.x - pos.x) + (center.y - pos.y)*(center.y - pos.y) + (center.z - pos.z)*(center.z - pos.z));

	return d_center - radius;
}

bool Sphere_Boundary::check_collision(XMFLOAT3 pos)
{
	if (calculate_distance(XMFLOAT3(-pos.x, -pos.y, -pos.z)) < radius)
		return true;

	return false;
}

void Sphere_Boundary::transform_boundary(XMMATRIX& M, float scale_factor)
{
	this->center = mul(this->center, M);
	this->radius *= scale_factor;
}



Box_Boundary::Box_Boundary(XMFLOAT3 max_x, XMFLOAT3 max_y, XMFLOAT3 max_z, XMFLOAT3 min_x, XMFLOAT3 min_y, XMFLOAT3 min_z)
	: max_x{ max_x }
	, max_y{ max_y }
	, max_z{ max_z }
	, min_x{ min_x }
	, min_y{ min_y }
	, min_z{ min_z }
{
}

//Box_Boundary::Box_Boundary(const Box_Boundary & other)
//{
//	this->max_x = other.max_x;
//	this->max_y = other.max_y;
//	this->max_z = other.max_z;
//	this->min_x = other.min_x;
//	this->min_y = other.min_y;
//	this->min_z = other.min_z;
//
//	this->
//}
//
//Boundary & Box_Boundary::operator=(const Boundary & other)
//{
//	const Box_Boundary *pBB = dynamic_cast<const Box_Boundary*>(&other);
//
//	this->max_x = pBB->max_x;
//	this->max_y = pBB->max_y;
//	this->max_z = pBB->max_z;
//	this->min_x = pBB->min_x;
//	this->min_y = pBB->min_y;
//	this->min_z = pBB->min_z;
//
//	return *this;
//}
//
//Box_Boundary & Box_Boundary::operator=(const Box_Boundary & other)
//{
//	this->max_x = other.max_x;
//	this->max_y = other.max_y;
//	this->max_z = other.max_z;
//	this->min_x = other.min_x;
//	this->min_y = other.min_y;
//	this->min_z = other.min_z;
//	
//	return *this;
//}


float Box_Boundary::calculate_distance(XMFLOAT3 pos)
{

	float d_max_x = sqrtf((max_x.x - pos.x)*(max_x.x - pos.x) + (max_x.y - pos.y)*(max_x.y - pos.y) + (max_x.z - pos.z)*(max_x.z - pos.z));
	float d_max_y = sqrtf((max_y.x - pos.x)*(max_y.x - pos.x) + (max_y.y - pos.y)*(max_y.y - pos.y) + (max_y.z - pos.z)*(max_y.z - pos.z));
	float d_max_z = sqrtf((max_z.x - pos.x)*(max_z.x - pos.x) + (max_z.y - pos.y)*(max_z.y - pos.y) + (max_z.z - pos.z)*(max_z.z - pos.z));

	float d_min_x = sqrtf((min_x.x - pos.x)*(min_x.x - pos.x) + (min_x.y - pos.y)*(min_x.y - pos.y) + (min_x.z - pos.z)*(min_x.z - pos.z));
	float d_min_y = sqrtf((min_y.x - pos.x)*(min_y.x - pos.x) + (min_y.y - pos.y)*(min_y.y - pos.y) + (min_y.z - pos.z)*(min_y.z - pos.z));
	float d_min_z = sqrtf((min_z.x - pos.x)*(min_z.x - pos.x) + (min_z.y - pos.y)*(min_z.y - pos.y) + (min_z.z - pos.z)*(min_z.z - pos.z));

	float distances[] = { d_max_x,d_max_y,d_max_z,d_min_x,d_min_y,d_min_z };

	return *std::max_element(distances, distances + 6);
}

bool Box_Boundary::check_collision(XMFLOAT3 pos)
{
	if (-pos.x < max_x.x && -pos.x > min_x.x
		&&  -pos.y < max_y.y && -pos.y > min_y.y
		&&	-pos.z < max_z.z && -pos.z > min_z.z)
	{
		return true;
	}

	return false;
}

void Box_Boundary::transform_boundary(XMMATRIX& M, float scale_factor)
{
	XMFLOAT3 maxx, maxy, maxz, minx, miny, minz;

	maxx = mul(this->max_x, M);
	maxy = mul(this->max_y, M);
	maxz = mul(this->max_z, M);
	minx = mul(this->min_x, M);
	miny = mul(this->min_y, M);
	minz = mul(this->min_z, M);

	XMFLOAT3 t[6] = { maxx,maxy,maxz,minx,miny,minz };

	for (int i = 0; i < 6; i++)
	{
		if (t[i].x > maxx.x)
		{
			maxx = t[i];
		}
		else if (t[i].x < minx.x)
		{
			minx = t[i];
		}

		if (t[i].y > maxy.y)
		{
			maxy = t[i];
		}
		else if (t[i].y < miny.y)
		{
			miny = t[i];
		}

		if (t[i].z > maxz.z)
		{
			maxz = t[i];
		}
		else if (t[i].z < minz.z)
		{
			minz = t[i];
		}
	}

	this->max_x = maxx;
	this->max_y = maxy;
	this->max_z = maxz;
	this->min_x = minx;
	this->min_y = miny;
	this->min_z = minz;

}

XMFLOAT3 mul(XMFLOAT3 vec, XMMATRIX &M)
{
	XMFLOAT3 erg;
	erg.x = M._11*vec.x + M._21*vec.y + M._31*vec.z + M._41*1.0;
	erg.y = M._12*vec.x + M._22*vec.y + M._32*vec.z + M._42*1.0;
	erg.z = M._13*vec.x + M._23*vec.y + M._33*vec.z + M._43*1.0;
	return erg;
}

float Coord_Boundary::calculate_distance(XMFLOAT3 pos)
{
	return 1000000.0f;
}

bool Coord_Boundary::check_collision(XMFLOAT3 pos)
{
	return false;
}

void Coord_Boundary::transform_boundary(XMMATRIX & M, float scale_factor)
{
}
