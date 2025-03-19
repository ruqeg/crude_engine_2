/////////////////////
//// @Scalar
/////////////////////
CRUDE_INLINE float32
crude_sin_cos
(
  _Out_ float32                                 *sinangle,
  _Out_ float32                                 *cosangle,
  _In_ float32                                   angle
)
{
  // thanks directxmath! look at XMScalarSinCos
  float32 quotient = CRUDE_C1DIV2PI * angle;
  if ( angle >= 0.0f )
  {
    quotient = CAST( float32, CAST( int32, quotient + 0.5f ) );
  }
  else
  {
    quotient = CAST( float32, CAST( int32, quotient - 0.5f ) );
  }
  float32 y = angle - CRUDE_C2PI * quotient;
  
  float32 sign;
  if ( y > CRUDE_CPI2 )
  {
    y = CRUDE_CPI - y;
    sign = -1.0f;
  }
  else if ( y < -CRUDE_CPI2 )
  {
    y = -CRUDE_CPI - y;
    sign = -1.0f;
  }
  else
  {
    sign = +1.0f;
  }
  
  float32 y2 = y * y;
  // 11-degree minimax approximation
  *sinangle = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;
  // 10-degree minimax approximation
  float32 p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
  *cosangle = sign * p;
}

/////////////////////
//// @Vector 3
/////////////////////
crude_vector
crude_vec_normalize3
(
  _In_ crude_vector const v
)
{
  crude_vector dot = _mm_dp_ps( v, v, 0x7f );
  crude_vector length = _mm_sqrt_ps( dot );
  crude_vector result = _mm_div_ps( v, length );
  return result;
}

crude_vector
crude_vec_cross3
(
  _In_ crude_vector const v1,
  _In_ crude_vector const v2
)
{
  crude_vector tmp1 = _mm_shuffle_ps( v1, v1, _MM_SHUFFLE( 3, 0, 2, 1 ) );
  crude_vector tmp2 = _mm_shuffle_ps( v2, v2, _MM_SHUFFLE( 3, 1, 0, 2 ) );
  crude_vector tmp3 = _mm_shuffle_ps( v1, v1, _MM_SHUFFLE( 3, 1, 0, 2 ) );
  crude_vector tmp4 = _mm_shuffle_ps( v2, v2, _MM_SHUFFLE( 3, 0, 2, 1 ) );
  
  crude_vector mul1 = _mm_mul_ps( tmp1, tmp2 );
  crude_vector mul2 = _mm_mul_ps( tmp3, tmp4 );

  crude_vector result = _mm_sub_ps( mul1, mul2 );
  return result;
}

/////////////////////
//// @Vector 4
/////////////////////
crude_vector
crude_vec_dot4
(
  _In_ crude_vector const                        v1,
  _In_ crude_vector const                        v2
)
{
  crude_vector result = _mm_dp_ps( v1, v2, 0xff );
  return result;
}

/////////////////////
//// @Matrix
/////////////////////

crude_matrix
crude_mat_perspective_fov_lh
(
  _In_ float32 fov_angle_y,
  _In_ float32 aspect_ratio,
  _In_ float32 nearz,
  _In_ float32 farz
)
{
  float32 cos_fov;
  float32 sin_fov;
  crude_sin_cos( &sin_fov, &cos_fov, 0.5f * fov_angle_y );
  
  float height = cos_fov / sin_fov;
  float width = height / aspect_ratio;
  float frange = farz / ( farz - nearz );
  
  crude_matrix m;
  m.r[ 0 ] = _mm_set_ps( 0.f, 0.f, 0.f, width );
  m.r[ 1 ] = _mm_set_ps( 0.f, 0.f, height, 0.f );
  m.r[ 2 ] = _mm_set_ps( 1.0f, frange, 0.f, width );
  m.r[ 3 ] = _mm_set_ps( 0.f, 0.f, -frange * nearz, 0.f );
  return m;
}

crude_matrix
crude_mat_inverse
(
  _Out_opt_ crude_vector                        *determinant,
  _In_ crude_matrix const                        m
)
{
  // I'm too lazy do it on my own... so for now I will just copy solution from XMMatrixInverse?
  crude_vector v_temp1 = _mm_shuffle_ps( m.r[ 0 ], m.r[ 1 ], _MM_SHUFFLE( 1, 0, 1, 0 ) );
  crude_vector v_temp3 = _mm_shuffle_ps( m.r[ 0 ], m.r[ 1 ], _MM_SHUFFLE( 3, 2, 3, 2 ) );
  crude_vector v_temp2 = _mm_shuffle_ps( m.r[ 2 ], m.r[ 3 ], _MM_SHUFFLE( 1, 0, 1, 0 ) );
  crude_vector v_temp4 = _mm_shuffle_ps( m.r[ 2 ], m.r[ 3 ], _MM_SHUFFLE( 3, 2, 3, 2 ) );

  crude_matrix mt;
  mt.r[ 0 ] = _mm_shuffle_ps( v_temp1, v_temp2, _MM_SHUFFLE( 2, 0, 2, 0 ) );
  mt.r[ 1 ] = _mm_shuffle_ps( v_temp1, v_temp2, _MM_SHUFFLE( 3, 1, 3, 1 ) );
  mt.r[ 2 ] = _mm_shuffle_ps( v_temp3, v_temp4, _MM_SHUFFLE( 2, 0, 2, 0 ) );
  mt.r[ 3 ] = _mm_shuffle_ps( v_temp3, v_temp4, _MM_SHUFFLE( 3, 1, 3, 1 ) );

  crude_vector v00 = _mm_shuffle_ps( mt.r[ 2 ], mt.r[ 2 ], _MM_SHUFFLE(1, 1, 0, 0) );
  crude_vector v10 = _mm_shuffle_ps( mt.r[ 3 ], mt.r[ 3 ], _MM_SHUFFLE(3, 2, 3, 2) );
  crude_vector v01 = _mm_shuffle_ps( mt.r[ 0 ], mt.r[ 0 ], _MM_SHUFFLE(1, 1, 0, 0) );
  crude_vector v11 = _mm_shuffle_ps( mt.r[ 1 ], mt.r[ 1 ], _MM_SHUFFLE(3, 2, 3, 2) );
  crude_vector v02 = _mm_shuffle_ps( mt.r[ 2 ], mt.r[ 0 ], _MM_SHUFFLE(2, 0, 2, 0) );
  crude_vector v12 = _mm_shuffle_ps( mt.r[ 3 ], mt.r[ 1 ], _MM_SHUFFLE(3, 1, 3, 1) );

  crude_vector d0 = _mm_mul_ps( v00, v10 );
  crude_vector d1 = _mm_mul_ps( v01, v11 );
  crude_vector d2 = _mm_mul_ps( v02, v12 );

  v00 = _mm_shuffle_ps( mt.r[ 2 ], mt.r[ 2 ], _MM_SHUFFLE( 3, 2, 3, 2 ) );
  v10 = _mm_shuffle_ps( mt.r[ 3 ], mt.r[ 3 ], _MM_SHUFFLE( 1, 1, 0, 0 ) );
  v01 = _mm_shuffle_ps( mt.r[ 0 ], mt.r[ 0 ], _MM_SHUFFLE( 3, 2, 3, 2 ) );
  v11 = _mm_shuffle_ps( mt.r[ 1 ], mt.r[ 1 ], _MM_SHUFFLE( 1, 1, 0, 0 ) );
  v02 = _mm_shuffle_ps( mt.r[ 2 ], mt.r[ 0 ], _MM_SHUFFLE( 3, 1, 3, 1 ) );
  v12 = _mm_shuffle_ps( mt.r[ 3 ], mt.r[ 1 ], _MM_SHUFFLE( 2, 0, 2, 0 ) );

  d0 = _mm_fnmadd_ps( v00, v10, d0 );
  d1 = _mm_fnmadd_ps( v01, v11, d1 );
  d2 = _mm_fnmadd_ps( v02, v12, d2 );

  v11 = _mm_shuffle_ps( d0, d2, _MM_SHUFFLE( 1, 1, 3, 1 ) );
  v00 = _mm_permute_ps( mt.r[ 1 ], _MM_SHUFFLE( 1, 0, 2, 1 ) );
  v10 = _mm_shuffle_ps( v11, d0, _MM_SHUFFLE( 0, 3, 0, 2 ) );
  v01 = _mm_permute_ps( mt.r[ 0 ], _MM_SHUFFLE( 0, 1, 0, 2 ) );
  v11 = _mm_shuffle_ps( v11, d0, _MM_SHUFFLE( 2, 1, 2, 1 ) );

  crude_vector v13 = _mm_shuffle_ps( d1, d2, _MM_SHUFFLE( 3, 3, 3, 1 ) );
  v02 = _mm_permute_ps( mt.r[ 3 ], _MM_SHUFFLE( 1, 0, 2, 1 ) );
  v12 = _mm_shuffle_ps( v13, d1, _MM_SHUFFLE( 0, 3, 0, 2 ) );
  crude_vector v03 = _mm_permute_ps( mt.r[ 2 ], _MM_SHUFFLE( 0, 1, 0, 2 ) );
  v13 = _mm_shuffle_ps( v13, d1, _MM_SHUFFLE( 2, 1, 2, 1 ) );
  
  crude_vector c0 = _mm_mul_ps( v00, v10 );
  crude_vector c2 = _mm_mul_ps( v01, v11 );
  crude_vector c4 = _mm_mul_ps( v02, v12 );
  crude_vector c6 = _mm_mul_ps( v03, v13 );
  
  v11 = _mm_shuffle_ps( d0, d2, _MM_SHUFFLE( 0, 0, 1, 0 ) );
  v00 = _mm_permute_ps( mt.r[ 1 ], _MM_SHUFFLE( 2, 1, 3, 2 ) );
  v10 = _mm_shuffle_ps( d0, v11, _MM_SHUFFLE( 2, 1, 0, 3 ) );
  v01 = _mm_permute_ps( mt.r[ 0 ], _MM_SHUFFLE( 1, 3, 2, 3 ) );
  v11 = _mm_shuffle_ps( d0, v11, _MM_SHUFFLE( 0, 2, 1, 2 ) );

  v13 = _mm_shuffle_ps( d1, d2, _MM_SHUFFLE( 2, 2, 1, 0 ) );
  v02 = _mm_permute_ps( mt.r[ 3 ], _MM_SHUFFLE( 2, 1, 3, 2 ) );
  v12 = _mm_shuffle_ps( d1, v13, _MM_SHUFFLE( 2, 1, 0, 3 ) );
  v03 = _mm_permute_ps( mt.r[ 2 ], _MM_SHUFFLE( 1, 3, 2, 3 ) );
  v13 = _mm_shuffle_ps( d1, v13, _MM_SHUFFLE( 0, 2, 1, 2 ) );
  
  c0 = _mm_fnmadd_ps( v00, v10, c0 );
  c2 = _mm_fnmadd_ps( v01, v11, c2 );
  c4 = _mm_fnmadd_ps( v02, v12, c4 );
  c6 = _mm_fnmadd_ps( v03, v13, c6 );
  
  v00 = _mm_permute_ps( mt.r[ 1 ], _MM_SHUFFLE( 0, 3, 0, 3 ) );

  v10 = _mm_shuffle_ps( d0, d2, _MM_SHUFFLE( 1, 0, 2, 2 ) );
  v10 = _mm_permute_ps( v10, _MM_SHUFFLE( 0, 2, 3, 0 ) );
  v01 = _mm_permute_ps( mt.r[ 0 ], _MM_SHUFFLE( 2, 0, 3, 1 ) );

  v11 = _mm_shuffle_ps( d0, d2, _MM_SHUFFLE( 1, 0, 3, 0 ) );
  v11 = _mm_permute_ps( v11, _MM_SHUFFLE( 2, 1, 0, 3 ) );
  v02 = _mm_permute_ps( mt.r[ 3 ], _MM_SHUFFLE( 0, 3, 0, 3 ) );

  v12 = _mm_shuffle_ps( d1, d2, _MM_SHUFFLE( 3, 2, 2, 2 ) );
  v12 = _mm_permute_ps( v12, _MM_SHUFFLE( 0, 2, 3, 0 ) );
  v03 = _mm_permute_ps( mt.r[ 2 ], _MM_SHUFFLE( 2, 0, 3, 1 ) );

  v13 = _mm_shuffle_ps( d1, d2, _MM_SHUFFLE( 3, 2, 3, 0 ) );
  v13 = _mm_permute_ps( v13, _MM_SHUFFLE( 2, 1, 0, 3 ) );
  
  v00 = _mm_mul_ps( v00, v10 );
  v01 = _mm_mul_ps( v01, v11 );
  v02 = _mm_mul_ps( v02, v12 );
  v03 = _mm_mul_ps( v03, v13 );
  crude_vector c1 = _mm_sub_ps( c0, v00 );
  c0 = _mm_add_ps( c0, v00 );
  crude_vector c3 = _mm_add_ps( c2, v01 );
  c2 = _mm_sub_ps( c2, v01);
  crude_vector c5 = _mm_sub_ps( c4, v02 );
  c4 = _mm_add_ps( c4, v02 );
  crude_vector c7 = _mm_add_ps( c6, v03 );
  c6 = _mm_sub_ps( c6, v03 );
  
  c0 = _mm_shuffle_ps( c0, c1, _MM_SHUFFLE( 3, 1, 2, 0 ) );
  c2 = _mm_shuffle_ps( c2, c3, _MM_SHUFFLE( 3, 1, 2, 0 ) );
  c4 = _mm_shuffle_ps( c4, c5, _MM_SHUFFLE( 3, 1, 2, 0 ) );
  c6 = _mm_shuffle_ps( c6, c7, _MM_SHUFFLE( 3, 1, 2, 0 ) );
  c0 = _mm_permute_ps( c0, _MM_SHUFFLE( 3, 1, 2, 0 ) );
  c2 = _mm_permute_ps( c2, _MM_SHUFFLE( 3, 1, 2, 0 ) );
  c4 = _mm_permute_ps( c4, _MM_SHUFFLE( 3, 1, 2, 0 ) );
  c6 = _mm_permute_ps( c6, _MM_SHUFFLE( 3, 1, 2, 0 ) );

  crude_vector v_temp = crude_vec_dot4( c0, mt.r[ 0 ] );
  if ( determinant )
  {
    *determinant= v_temp;
  }

  v_temp = _mm_div_ps( CRUDE_MATH_VECTOR_ONE.v, v_temp );
  
  crude_matrix mResult;
  mResult.r[ 0 ] = _mm_mul_ps( c0, v_temp );
  mResult.r[ 1 ] = _mm_mul_ps( c2, v_temp );
  mResult.r[ 2 ] = _mm_mul_ps( c4, v_temp );
  mResult.r[ 3 ] = _mm_mul_ps( c6, v_temp );
  return mResult;
}

/////////////////////
//// @Convert
/////////////////////
void
crude_store_float4x4
(
  _Out_ crude_float4x4    *f,
  _In_ crude_matrix const  m
)
{
  _mm_storeu_ps( &f->_01, m.r[ 0 ] );
  _mm_storeu_ps( &f->_11, m.r[ 1 ] );
  _mm_storeu_ps( &f->_21, m.r[ 2 ] );
  _mm_storeu_ps( &f->_31, m.r[ 3 ] );
}

void
crude_store_float4x4a
(
  _Out_ crude_float4x4a   *f,
  _In_ crude_matrix const  m
)
{
  _mm_store_ps( &f->_01, m.r[ 0 ] );
  _mm_store_ps( &f->_11, m.r[ 1 ] );
  _mm_store_ps( &f->_21, m.r[ 2 ] );
  _mm_store_ps( &f->_31, m.r[ 3 ] );
}