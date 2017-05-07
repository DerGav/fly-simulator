#include "groundwork.h"

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
		this->boundary = new Coord_Boundary(*pCB);

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

	//load 3ds file
	Load3DS(filename, device, &vertex_buffer, &vertex_count, b, this, g);

}


Sphere_Boundary::Sphere_Boundary(XMFLOAT3 center, float radius) : center{ center }, radius{ radius }
{
}



Coord_Boundary::Coord_Boundary() : vertices{ NULL }, num_vertices{ 0 }
{}

Coord_Boundary::Coord_Boundary(const Coord_Boundary& other)
{
	this->vertices = new SimpleVertex[other.num_vertices];
	for (int i = 0; i < other.num_vertices; i++)
	{
		this->vertices[i] = other.vertices[i];
	}
	this->num_vertices = other.num_vertices;
}

Coord_Boundary Coord_Boundary::operator=(const Coord_Boundary& rhs)
{
	for (int i = 0; i < this->num_vertices; i++)
	{
		this->vertices[i] = rhs.vertices[i];
	}
	this->num_vertices = rhs.num_vertices;

	return *this;
}

Coord_Boundary::~Coord_Boundary() 
{
	delete[] this->vertices;
}

float Sphere_Boundary::calculate_distance(XMFLOAT3 pos)
{
	float d_center = sqrtf((center.x - pos.x)*(center.x - pos.x) + (center.y - pos.y)*(center.y - pos.y) + (center.z - pos.z)*(center.z - pos.z));

	return d_center - radius;
}

bool Sphere_Boundary::check_collision(XMFLOAT3 pos, XMFLOAT3 camPos)
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
{}

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

bool Box_Boundary::check_collision(XMFLOAT3 pos, XMFLOAT3 camPos)
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
	return 1000000000.0f;
}

bool Coord_Boundary::check_collision(XMFLOAT3 pos, XMFLOAT3 camPos)
{
	XMFLOAT3 intersect_point,P0,P1,d_vec;
	float d;
	for (int i = 0; i < this->num_vertices; i = i + 3)
	{
		P0 = XMFLOAT3(-camPos.x, -camPos.y, -camPos.z);
		P1 = XMFLOAT3(-pos.x, -pos.y, -pos.z);
		if (intersect3D_RayTriangle(P0, P1, this->vertices[i].Pos, this->vertices[i + 1].Pos, this->vertices[i + 2].Pos, intersect_point) == 1)
			d_vec = intersect_point - camPos;
			d = sqrt(d_vec.x*d_vec.x + d_vec.y*d_vec.y + d_vec.z*d_vec.z);
			if (d < 90)
			return true;
	}

	return false;
}

void Coord_Boundary::transform_boundary(XMMATRIX & M, float scale_factor)
{
	for (int i = 0; i < this->num_vertices; i++)
	{
		this->vertices[i].Pos  = mul(this->vertices[i].Pos,  M);
		this->vertices[i].Norm = mul(this->vertices[i].Norm, M);
	}
}

#define SMALL_NUM   0.00000001 // anything that avoids division overflow
// intersect3D_RayTriangle(): find the 3D intersection of a line segment with a triangle
//    Input:   2 points P0,P1 that define a line segment, and 3 points defining a triangle
//    Output:  I = intersection point (when it exists)
//    Return: -1 = triangle is degenerate (a segment or point)
//             0 =  disjoint (no intersect)
//             1 =  intersect in unique point I1
//             2 =  are in the same plane
int intersect3D_RayTriangle(const XMFLOAT3 P0, const XMFLOAT3 P1, const XMFLOAT3 V0, const XMFLOAT3 V1, const XMFLOAT3 V2, XMFLOAT3& I)
{
	XMVECTOR    u, v, n;              // triangle vectors
	XMVECTOR    dir, w0, w;           // ray vectors
	XMVECTOR    null_vector = XMLoadFloat3(&XMFLOAT3(0, 0, 0));
	float     r, a, b;              // params to calc ray-plane intersect

	// get triangle edge vectors and plane normal
	//XMFLOAT3 t0 = V1 - V0;
	//XMFLOAT3 t1 = V2 - V0;
	u = XMLoadFloat3(&(V1 - V0));
	v = XMLoadFloat3(&(V2 - V0));
	n = XMVector3Cross(u, v);		// cross product

	UINT comparisonResult; 
	XMVectorEqualR(&comparisonResult, n, null_vector);
	if (XMComparisonAllTrue(comparisonResult))             // triangle is degenerate
		return -1;                  // do not deal with this case

	dir = XMLoadFloat3(&(P1 - P0));              // ray direction vector
	float dir_len = XMVectorGetX(XMVector3Length(dir));
	w0 = XMLoadFloat3(&(P0 - V0));

	a = - XMVectorGetX( XMVector3Dot(n, w0));
	b = XMVectorGetX(XMVector3Dot(n, dir));
	if (fabs(b) < SMALL_NUM) {     // ray is  parallel to triangle plane
		if (a == 0)                 // ray lies in triangle plane
			return 2;
		else return 0;              // ray disjoint from plane
	}

	// get intersect point of ray with triangle plane
	r = a / b;
	if (r < 0.0 || 1.0 < r)                    // ray goes away from triangle
		return 0;                   // => no intersect
	// for a segment, also test if (r > 1.0) => no intersect

	XMFLOAT3 dir_f3;
	XMStoreFloat3(&dir_f3, dir);
	I = P0 + XMFLOAT3(r * dir_f3.x, r * dir_f3.y, r * dir_f3.z);// intersect point of ray and plane

	// is I inside T?
	float    uu, uv, vv, wu, wv, D;
	uu = XMVectorGetX(XMVector3Dot(u, u));
	uv = XMVectorGetX(XMVector3Dot(u, v));
	vv = XMVectorGetX(XMVector3Dot(v, v));
	w =	 XMLoadFloat3(&(I - V0));
	wu = XMVectorGetX(XMVector3Dot(w, u));
	wv = XMVectorGetX(XMVector3Dot(w, v));
	D = uv * uv - uu * vv;

	// get and test parametric coords
	float s, t;
	s = (uv * wv - vv * wu) / D;
	if (s < 0.0 || s > 1.0)         // I is outside T
		return 0;
	t = (uv * wu - uu * wv) / D;
	if (t < 0.0 || (s + t) > 1.0)  // I is outside T
		return 0;

	return 1;                       // I is in T
}