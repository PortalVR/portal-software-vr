// D3DCamera.h - viewpoint camera class declaration
#pragma once

#include "D3Dutility.h"

enum ECameraType { LANDOBJECT, AIRCRAFT };

class CD3DCamera
{
protected:
	ECameraType		m_eCameraType;
	D3DXVECTOR3		m_pos;
	D3DXVECTOR3		m_right;
	D3DXVECTOR3		m_up;
	D3DXVECTOR3		m_look;
	float			m_height;

public:
	CD3DCamera ();
	CD3DCamera (ECameraType eCameraType);
	~CD3DCamera ();

	void setCameraType (ECameraType eCameraType);
	void SetCameraHeight (float height);
	void setPosition (const D3DXVECTOR3 &pos);
	void setLook (const D3DXVECTOR3 &look);

	void strafe (float units); // left/right
	void fly (float units);    // up/down
	void walk (float units);   // forward/backward

	void pitch (float angle); // rotate on right vector
	void yaw (float angle);   // rotate on up vector
	void roll (float angle);  // rotate on look vector

	ECameraType getCameraType () const; 
	float GetCameraHeight () const;
	void getPosition (D3DXVECTOR3 &pos) const; 
	void getRight (D3DXVECTOR3 &right) const;
	void getUp (D3DXVECTOR3 &up) const;
	void getLook (D3DXVECTOR3 &look) const;

	void getViewMatrix (D3DXMATRIX &view); 
};
