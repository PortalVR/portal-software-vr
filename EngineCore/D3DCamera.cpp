// D3DCamera.h - viewpoint camera class definitions
#include "D3DCamera.h"

CD3DCamera::CD3DCamera ()
{
	m_eCameraType = AIRCRAFT;

	m_pos   = D3DXVECTOR3 (0.0f, 0.0f, 0.0f);
	m_right = D3DXVECTOR3 (1.0f, 0.0f, 0.0f);
	m_look  = D3DXVECTOR3 (0.0f, 0.0f, 1.0f);
	m_up    = D3DXVECTOR3 (0.0f, 1.0f, 0.0f);

	m_height = 2.0f;
}

CD3DCamera::CD3DCamera (ECameraType eCameraType)
{
	m_eCameraType = eCameraType;

	m_pos   = D3DXVECTOR3 (0.0f, 0.0f, 0.0f);
	m_right = D3DXVECTOR3 (1.0f, 0.0f, 0.0f);
	m_look  = D3DXVECTOR3 (0.0f, 0.0f, 1.0f);
	m_up    = D3DXVECTOR3 (0.0f, 1.0f, 0.0f);

	m_height = 2.0f;
}

CD3DCamera::~CD3DCamera ()
{
}

void CD3DCamera::setCameraType (ECameraType eCameraType)
{
	m_eCameraType = eCameraType;
}

void CD3DCamera::SetCameraHeight (float height)
{
	m_height = height;
}

void CD3DCamera::setPosition (const D3DXVECTOR3 &pos)
{
	m_pos = pos;
}

void CD3DCamera::setLook (const D3DXVECTOR3 &look)
{
	m_look = look;
}

void CD3DCamera::strafe (float units)
{
	// move only on xz plane for land object
	if (m_eCameraType == LANDOBJECT)
		m_pos += D3DXVECTOR3 (m_right.x, 0.0f, m_right.z) * units;

	if (m_eCameraType == AIRCRAFT)
		m_pos += m_right * units;
}

void CD3DCamera::fly (float units)
{
	// move only on y-axis for land object
	if (m_eCameraType == LANDOBJECT)
		m_pos.y += units;

	if (m_eCameraType == AIRCRAFT)
		m_pos += m_up * units;
}

void CD3DCamera::walk (float units)
{
	// move only on xz plane for land object
	if (m_eCameraType == LANDOBJECT)
		m_pos += D3DXVECTOR3 (m_look.x, 0.0f, m_look.z) * units;

	if (m_eCameraType == AIRCRAFT)
		m_pos += m_look * units;
}

void CD3DCamera::pitch (float angle)
{
	D3DXMATRIX T;
	D3DXMatrixRotationAxis (&T, &m_right, angle);

	// rotate _up and _look around _right vector
	D3DXVec3TransformCoord (&m_up, &m_up, &T);
	D3DXVec3TransformCoord (&m_look, &m_look, &T);
}

void CD3DCamera::yaw (float angle)
{
	D3DXMATRIX T;

	// rotate around world y (0, 1, 0) always for land object
	if (m_eCameraType == LANDOBJECT)
		D3DXMatrixRotationY (&T, angle);

	// rotate around own up vector for aircraft
	if (m_eCameraType == AIRCRAFT)
		D3DXMatrixRotationAxis (&T, &m_up, angle);

	// rotate _right and _look around _up or y-axis
	D3DXVec3TransformCoord (&m_right, &m_right, &T);
	D3DXVec3TransformCoord (&m_look, &m_look, &T);
}

void CD3DCamera::roll (float angle)
{
	// only roll for aircraft type
	if (m_eCameraType == AIRCRAFT)
	{
		D3DXMATRIX T;
		D3DXMatrixRotationAxis (&T, &m_look, angle);

		// rotate _up and _right around _look vector
		D3DXVec3TransformCoord (&m_right, &m_right, &T);
		D3DXVec3TransformCoord (&m_up, &m_up, &T);
	}
}

ECameraType CD3DCamera::getCameraType () const
{
	return m_eCameraType;
}

float CD3DCamera::GetCameraHeight () const
{
	return m_height;
}

void CD3DCamera::getPosition (D3DXVECTOR3 &pos) const
{
	pos = m_pos;
}

void CD3DCamera::getRight (D3DXVECTOR3 &right) const
{
	right = m_right;
}

void CD3DCamera::getUp (D3DXVECTOR3 &up) const
{
	up = m_up;
}

void CD3DCamera::getLook (D3DXVECTOR3 &look) const
{
	look = m_look;
}

void CD3DCamera::getViewMatrix (D3DXMATRIX &view)
{
	// Keep camera's axes orthogonal to eachother
	D3DXVec3Normalize (&m_look, &m_look);

	D3DXVec3Cross (&m_up, &m_look, &m_right);
	D3DXVec3Normalize (&m_up, &m_up);

	D3DXVec3Cross (&m_right, &m_up, &m_look);
	D3DXVec3Normalize (&m_right, &m_right);

	// Build the view matrix:
	float x = -D3DXVec3Dot (&m_right, &m_pos);
	float y = -D3DXVec3Dot (&m_up, &m_pos);
	float z = -D3DXVec3Dot (&m_look, &m_pos);

	// D3D uses LH Coordinate System
	view(0,0) = m_right.x; view(0, 1) = m_up.x; view(0, 2) = m_look.x; view(0, 3) = 0.0f;
	view(1,0) = m_right.y; view(1, 1) = m_up.y; view(1, 2) = m_look.y; view(1, 3) = 0.0f;
	view(2,0) = m_right.z; view(2, 1) = m_up.z; view(2, 2) = m_look.z; view(2, 3) = 0.0f;
	view(3,0) = x;         view(3, 1) = y;      view(3, 2) = z;        view(3, 3) = 1.0f;
}
