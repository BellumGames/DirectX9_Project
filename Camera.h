class CXCamera
{
private:
protected:
	D3DXVECTOR3 m_Position;
	D3DXVECTOR3 m_LookAt;
	D3DXVECTOR3 m_Right;
	D3DXVECTOR3 m_Up;
	
	float m_fRotAboutUp;
    float m_fRotAboutRight;
    float m_fRotAboutFacing;

	D3DXMATRIX m_ViewTransform;

	LPDIRECT3DDEVICE9 m_pDevice;
	bool m_UpdateRequired;

	HRESULT UpdateCameraMatrices();
public:
	void LookAtPos(D3DXVECTOR3 *Position, D3DXVECTOR3 *LookAt, D3DXVECTOR3 *Up);
	void SetPosition(FLOAT X, FLOAT Y, FLOAT Z);
	D3DXVECTOR3* GetPosition() {return &m_Position;}
	D3DXVECTOR3* GetLookAt() {return &m_LookAt;}
	D3DXVECTOR3* GetRight() {return &m_Right;}
	D3DXVECTOR3* GetUp() {return &m_Up;}
	D3DXMATRIX* GetViewMatrix() {return &m_ViewTransform;}
	CXCamera(LPDIRECT3DDEVICE9 pDevice);
	HRESULT Update();

	void RotateDown(float fAngle);
	void RotateRight(float fAngle);
	void Roll(float fAngle);
	void MoveForward(float fDist);
	void MoveRight(float fDist);
	void MoveUp(float fDist);
	void MoveInDirection(float fDist, D3DXVECTOR3* Dir);
};

//---------------------------------------------------------------------------------

CXCamera::CXCamera(LPDIRECT3DDEVICE9 pDevice)
{
	m_Position = D3DXVECTOR3(0.0f,0.0f,0.0f);
	m_LookAt = D3DXVECTOR3(0.0f,0.0f,1.0f);
	m_Right = D3DXVECTOR3(1.0f,0.0f,0.0f);
	m_Up = D3DXVECTOR3(0.0f,1.0f,0.0f);
	m_UpdateRequired = false;

	m_fRotAboutUp = m_fRotAboutRight = m_fRotAboutFacing = 0.0f;
	D3DXMatrixIdentity(&m_ViewTransform);

	m_pDevice = pDevice;
}

HRESULT CXCamera::Update()
{
	if(m_pDevice)
	{
		if(m_UpdateRequired)
			return UpdateCameraMatrices();

		return m_pDevice->SetTransform(D3DTS_VIEW, &m_ViewTransform);
	}

	return E_FAIL;
}

HRESULT CXCamera::UpdateCameraMatrices()
{
	D3DXMATRIX matTotal;
	D3DXMATRIX matRotAboutUp, matRotAboutRight, matRotAboutFacing;

	D3DXMatrixRotationAxis(&matRotAboutRight,&m_Right,m_fRotAboutRight);
    D3DXMatrixRotationAxis(&matRotAboutUp,&m_Up,m_fRotAboutUp);
    D3DXMatrixRotationAxis(&matRotAboutFacing,&m_LookAt,m_fRotAboutFacing);
	
	D3DXMatrixMultiply(&matTotal,&matRotAboutUp,&matRotAboutRight);
    D3DXMatrixMultiply(&matTotal,&matRotAboutFacing,&matTotal);

	D3DXVec3TransformCoord(&m_Right,&m_Right,&matTotal);   
    D3DXVec3TransformCoord(&m_Up,&m_Up,&matTotal);   
    D3DXVec3Cross(&m_LookAt,&m_Right,&m_Up);

	if(fabs(D3DXVec3Dot(&m_Up,&m_Right)) > 0.01)
	{      
		D3DXVec3Cross(&m_Up,&m_LookAt,&m_Right);
    }

	D3DXVec3Normalize(&m_Right,&m_Right);
    D3DXVec3Normalize(&m_Up,&m_Up);
    D3DXVec3Normalize(&m_LookAt,&m_LookAt);

	float fView41,fView42,fView43;
    fView41 = -D3DXVec3Dot(&m_Right,&m_Position);
    fView42 = -D3DXVec3Dot(&m_Up,&m_Position);
    fView43 = -D3DXVec3Dot(&m_LookAt,&m_Position);

	m_ViewTransform = D3DXMATRIX(m_Right.x, m_Up.x, m_LookAt.x, 0.0f,
								 m_Right.y, m_Up.y, m_LookAt.y, 0.0f,
                                 m_Right.z, m_Up.z, m_LookAt.z, 0.0f,
                                 fView41, fView42,  fView43,	1.0f);

	m_fRotAboutUp = m_fRotAboutRight = m_fRotAboutFacing = 0.0f;
	m_UpdateRequired = false;

	return m_pDevice->SetTransform(D3DTS_VIEW, &m_ViewTransform);
}

void CXCamera::LookAtPos(D3DXVECTOR3 *Position, D3DXVECTOR3 *LookAt, D3DXVECTOR3 *Up)
{
	D3DXMatrixLookAtLH(&m_ViewTransform, Position, LookAt, Up);

	m_Position = *(Position);

	m_Right.x = m_ViewTransform._11;
	m_Right.y = m_ViewTransform._21;
	m_Right.z = m_ViewTransform._31;

	m_Up.x = m_ViewTransform._12;
	m_Up.y = m_ViewTransform._22;
	m_Up.z = m_ViewTransform._32;

	m_LookAt.x = m_ViewTransform._13;
	m_LookAt.y = m_ViewTransform._23;
	m_LookAt.z = m_ViewTransform._33;

	m_fRotAboutUp = m_fRotAboutRight = m_fRotAboutFacing = 0.0f;
}

void CXCamera::SetPosition(FLOAT X, FLOAT Y, FLOAT Z)
{
	m_Position = D3DXVECTOR3(X, Y, Z);
	m_UpdateRequired = true;
}

void CXCamera::RotateDown(float fAngle)
{
	m_fRotAboutRight += fAngle;
	m_UpdateRequired = true;
}

void CXCamera::RotateRight(float fAngle)
{
	m_fRotAboutUp += fAngle;
	m_UpdateRequired = true;
}

void CXCamera::Roll(float fAngle)
{
	m_fRotAboutFacing += fAngle;
	m_UpdateRequired = true;
}

void CXCamera::MoveForward(float fDist)
{
	m_Position += fDist * m_LookAt;
	m_UpdateRequired = true;
}

void CXCamera::MoveRight(float fDist)
{
	m_Position += fDist * m_Right;
	m_UpdateRequired = true;
}

void CXCamera::MoveUp(float fDist)
{
	m_Position += fDist * m_Up;
	m_UpdateRequired = true;
}

void CXCamera::MoveInDirection(float fDist, D3DXVECTOR3* Dir)
{
	D3DXVECTOR3 DirToMove(0,0,0);
	D3DXVec3Normalize(&DirToMove, Dir);
	m_Position += fDist*DirToMove;
	m_UpdateRequired = true;
}
