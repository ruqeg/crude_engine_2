/************************************************
 *
 * Scalar Functinos
 * 
 ***********************************************/
float32
crude_max
(
  _In_ float32                                   s1,
  _In_ float32                                   s2
)
{
  return s1 > s2 ? s1 : s2;
}

float32
crude_clamp
(
  _In_ float32                                   s,
  _In_ float32                                   min,
  _In_ float32                                   max
)
{
  return ( s  < min ) ? ( min ) : ( max < s ) ? max : s;
}

float32
crude_lerp
(
  _In_ float32                                   a,
  _In_ float32                                   b,
  _In_ float32                                   f
)
{
  return a + f * ( b - a );
}

float32
crude_pow
(
  _In_ float32                                   s1,
  _In_ float32                                   s2
)
{
  return powf( s1, s2 );
}

void
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
    quotient = ( int32 )( quotient + 0.5f );
  }
  else
  {
    quotient = ( int32 )( quotient - 0.5f );
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

/************************************************
 *
 * Vector Common Int Functinos
 * 
 ***********************************************/

// !TODO

/************************************************
 *
 * Vector Common Float Functinos
 * 
 ***********************************************/
crude_vector
crude_vec_select
(
  _In_ crude_vector const                        v1,
  _In_ crude_vector const                        v2,
  _In_ crude_vector const                        control
)
{
  return _mm_blendv_ps( v1, v2, control );
}

crude_vector
crude_vec_zero
(
)
{
  return _mm_set_ps( 0.0f, 0.0f, 0.0f, 0.0f  );
}

crude_vector
crude_vec_set
(
  _In_ float32                                   x,
  _In_ float32                                   y,
  _In_ float32                                   z,
  _In_ float32                                   w
)
{
  return _mm_set_ps( w, z, y, x );
}

crude_vector
crude_vec_replicate
(
  _In_ float32                                   value
)
{
  return _mm_set_ps1( value );
}

float32
crude_vec_get_x
( 
  _In_ crude_vector const                        v
)
{
  return _mm_cvtss_f32( v );
}

float32
crude_vec_get_y                                  
(
  _In_ crude_vector const                        v
)
{
  crude_vector vTemp = _mm_permute_ps( v, _MM_SHUFFLE( 1, 1, 1, 1 ) );
  return _mm_cvtss_f32(vTemp);
}

float32
crude_vec_get_z
( 
  _In_ crude_vector const                        v
)
{
  crude_vector vTemp = _mm_permute_ps( v, _MM_SHUFFLE( 2, 2, 2, 2 ) );
  return _mm_cvtss_f32(vTemp);
}

float32
crude_vec_get_w
(
  _In_ crude_vector const                        v
)
{
  crude_vector vTemp = _mm_permute_ps( v, _MM_SHUFFLE( 3, 3, 3, 3 ) );
  return _mm_cvtss_f32(vTemp);
}

crude_vector
crude_vec_negate
(
  _In_ crude_vector const                        v
)
{
  crude_vector z;
  z = _mm_setzero_ps();
  return _mm_sub_ps( z, v );
}

crude_vector
crude_vec_add
(
  _In_ crude_vector const                        v1,
  _In_ crude_vector const                        v2
)
{
  return _mm_add_ps( v1, v2 );
}

crude_vector
crude_vec_subtract
(
  _In_ crude_vector const                        v1,
  _In_ crude_vector const                        v2
)
{
  return _mm_sub_ps( v1, v2 );
}

crude_vector
crude_vec_multiply
(
  _In_ crude_vector const                        v1,
  _In_ crude_vector const                        v2
)
{
  return _mm_mul_ps( v1, v2 );
}

crude_vector
crude_vec_scale
( 
  _In_ crude_vector const                        v,
  _In_ float32                                   s
)
{
  crude_vector vResult = _mm_set_ps1( s );
  return _mm_mul_ps(vResult, v );
}

crude_vector
crude_vec_sqrt
(
  _In_ crude_vector const                        v
)
{
  return _mm_sqrt_ps( v );
}

crude_vector
crude_vec_abs
(
  _In_ crude_vector const                        v
)
{
  crude_vector result = _mm_setzero_ps();
  result = _mm_sub_ps( result, v );
  result = _mm_max_ps( result, v );
  return result;
}

crude_vector
crude_vec_scale_add
(
  _In_ crude_vector const                        v1,
  _In_ crude_vector const                        v2,
  _In_ float32                                   s
)
{
  crude_vector scalar = _mm_set_ps1( s );
  crude_vector v2_scaled = _mm_mul_ps( scalar, v2 );
  crude_vector result = _mm_add_ps( v1, v2_scaled );
  return result;
}

crude_vector
crude_vec_sin_cos
(
  _Out_ crude_vector                            *sin,
  _Out_ crude_vector                            *cos,
  _In_ crude_vector const                        v
)
{
  *sin = _mm_sincos_ps( cos, v );
}

crude_vector
crude_vec_permute
(
  _In_ crude_vector                              v1, 
  _In_ crude_vector                              v2,
  _In_ uint32                                    permute_x,
  _In_ uint32                                    permute_y,
  _In_ uint32                                    permute_z,
  _In_ uint32                                    permute_w 
)
{
  //uint32 Shuffle = _MM_SHUFFLE( permute_w & 3, permute_z & 3, permute_y & 3, permute_x & 3);
  //
  //bool WhichX = permute_x > 3;
  //bool WhichY = permute_y > 3;
  //bool WhichZ = permute_z > 3;
  //bool WhichW = permute_w > 3;
  //
  //crude_vector selectMask = _mm_castsi128_ps( _mm_set_epi32( WhichX ? 0xFFFFFFFF : 0, WhichY ? 0xFFFFFFFF : 0, WhichZ ? 0xFFFFFFFF : 0, WhichW ? 0xFFFFFFFF : 0 ) );
  //
  //crude_vector shuffled1 = _mm_permute_ps(v1, Shuffle);
  //crude_vector shuffled2 = _mm_permute_ps(v2, Shuffle);
  //
  //crude_vector masked1 = _mm_andnot_ps(selectMask, shuffled1);
  //crude_vector masked2 = _mm_and_ps(selectMask, shuffled2);
  //
  //return _mm_or_ps(masked1, masked2);
}

CRUDE_INLINE crude_vector
crude_vec_default_basis_forward
(
)
{
  return _mm_set_ps( 0, 1, 0, 0 );
}

CRUDE_INLINE crude_vector
crude_vec_default_basis_right
(
)
{
  return _mm_set_ps( 0, 0, 0, 1 );
}

CRUDE_INLINE crude_vector
crude_vec_default_basis_up
(
)
{
  return _mm_set_ps( 0, 0, 1, 0 );
}


/************************************************
 *
 * Vector 1th Functions
 * 
 ***********************************************/


/************************************************
 *
 * Vector 2th Functions
 * 
 ***********************************************/


/************************************************
 *
 * Vector 3th Functions
 * 
 ***********************************************/
crude_vector
crude_vec_transform_normal3
(
  _In_ crude_vector                              v, 
  _In_ crude_matrix                              m
)
{
  crude_vector vResult = _mm_permute_ps(v,_MM_SHUFFLE(2,2,2,2));
  vResult = _mm_mul_ps( vResult, m.r[2] );
  crude_vector vTemp = _mm_permute_ps(v,_MM_SHUFFLE(1,1,1,1));
  vResult = _mm_fmadd_ps( vTemp, m.r[1], vResult );
  vTemp = _mm_permute_ps(v,_MM_SHUFFLE(0,0,0,0));
  vResult = _mm_fmadd_ps( vTemp, m.r[0], vResult );
  return vResult;
}

crude_vector
crude_vec_dot3
(
  _In_ crude_vector const                        v1,
  _In_ crude_vector const                        v2
)
{
  return _mm_dp_ps( v1, v2, 0x7f );
}

crude_vector
crude_vec_length3
(
  _In_ crude_vector const                       v
)
{
  crude_vector result;
  result = crude_vec_dot3( v, v );
  result = crude_vec_sqrt( result );
  
  return result;
}

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

/************************************************
 *
 * Vector 4h Functions
 * 
 ***********************************************/
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

/************************************************
 *
 * Quaternion functions
 * 
 ***********************************************/
crude_vector 
crude_quat_identity
(
)
{
  crude_vector res = _mm_set_ps( 1.0f, 0.0f, 0.0f, 0.0f );
  return res;
}

crude_vector 
crude_quat_multiply
(
  _In_ crude_vector                              q1,
  _In_ crude_vector                              q2
)
{
  crude_vector ControlWZYX = _mm_setr_ps( 1.0f, -1.0f, 1.0f, -1.0f );
  crude_vector ControlZWXY = _mm_setr_ps( 1.0f, 1.0f, -1.0f, -1.0f );
  crude_vector ControlYXWZ = _mm_setr_ps( -1.0f, 1.0f, 1.0f, -1.0f );
  // Copy to SSE registers and use as few as possible for x86
  crude_vector Q2X = q2;
  crude_vector Q2Y = q2;
  crude_vector Q2Z = q2;
  crude_vector vResult = q2;
  // Splat with one instruction
  vResult = _mm_permute_ps(vResult, _MM_SHUFFLE(3, 3, 3, 3));
  Q2X = _mm_permute_ps(Q2X, _MM_SHUFFLE(0, 0, 0, 0));
  Q2Y = _mm_permute_ps(Q2Y, _MM_SHUFFLE(1, 1, 1, 1));
  Q2Z = _mm_permute_ps(Q2Z, _MM_SHUFFLE(2, 2, 2, 2));
  // Retire Q1 and perform Q1*Q2W
  vResult = _mm_mul_ps(vResult, q1);
  crude_vector Q1Shuffle = q1;
  // Shuffle the copies of Q1
  Q1Shuffle = _mm_permute_ps(Q1Shuffle, _MM_SHUFFLE(0, 1, 2, 3));
  // Mul by Q1WZYX
  Q2X = _mm_mul_ps(Q2X, Q1Shuffle);
  Q1Shuffle = _mm_permute_ps(Q1Shuffle, _MM_SHUFFLE(2, 3, 0, 1));
  // Flip the signs on y and z
  vResult = _mm_fmadd_ps(Q2X, ControlWZYX, vResult);
  // Mul by Q1ZWXY
  Q2Y = _mm_mul_ps(Q2Y, Q1Shuffle);
  Q1Shuffle = _mm_permute_ps(Q1Shuffle, _MM_SHUFFLE(0, 1, 2, 3));
  // Flip the signs on z and w
  Q2Y = _mm_mul_ps(Q2Y, ControlZWXY);
  // Mul by Q1YXWZ
  Q2Z = _mm_mul_ps(Q2Z, Q1Shuffle);
  // Flip the signs on x and w
  Q2Y = _mm_fmadd_ps(Q2Z, ControlYXWZ, Q2Y);
  vResult = _mm_add_ps(vResult, Q2Y);
  return vResult;
}

crude_vector 
crude_quat_rotation_axis
(
  _In_ crude_vector                              axis,
  _In_ float32                                   angle
)
{
  crude_vector Normal = crude_vec_normalize3( axis);
  crude_vector Q = crude_quat_rotation_normal( Normal, angle );
  return Q;
}

crude_vector 
crude_quat_rotation_normal
(
  _In_ crude_vector                              normal,
  _In_ float32                                   angle
)
{
  crude_vector N = _mm_and_ps( normal, CRUDE_MATH_MASK_3 );
  N = _mm_or_ps( N, CRUDE_MATH_IDENTITY_R3 );
  crude_vector Scale = _mm_set_ps1(0.5f * angle);
  crude_vector vSine;
  crude_vector vCosine;
  crude_vec_sin_cos( &vSine, &vCosine, Scale );
  Scale = _mm_and_ps( vSine, CRUDE_MATH_MASK_3 );
  vCosine = _mm_and_ps( vCosine, CRUDE_MATH_MASK_W );
  Scale = _mm_or_ps( Scale, vCosine );
  N = _mm_mul_ps( N, Scale );
  return N;
}

crude_vector
crude_quat_rotation_roll_pitch_yaw
(
  _In_ float32                                   pitch,
  _In_ float32                                   yaw,
  _In_ float32                                   roll
)
{
  crude_vector angles = crude_vec_set( pitch, yaw, roll, 0.0f );
  return crude_quat_rotation_roll_pitch_yaw_from_vector( angles );
}

crude_vector
crude_quat_rotation_roll_pitch_yaw_from_vector
(
  _In_ crude_vector const                        angles
)
{
  crude_vector Sign = _mm_set_ps( 1.0f, -1.0f, -1.0f, 1.0f );
  
  crude_vector HalfAngles = crude_vec_multiply( angles, CRUDE_MATH_ONE_HALF);
  
  crude_vector SinAngles, CosAngles;
  crude_vec_sin_cos(&SinAngles, &CosAngles, HalfAngles);
  
  crude_vector P0 = crude_vec_permute( SinAngles, CosAngles, 0, 4, 4, 4 );
  crude_vector Y0 = crude_vec_permute( SinAngles, CosAngles, 5, 1, 5, 5 );
  crude_vector R0 = crude_vec_permute( SinAngles, CosAngles, 6, 6, 2, 6 );
  crude_vector P1 = crude_vec_permute( CosAngles, SinAngles, 0, 4, 4, 4 );
  crude_vector Y1 = crude_vec_permute( CosAngles, SinAngles, 5, 1, 5, 5 );
  crude_vector R1 = crude_vec_permute( CosAngles, SinAngles, 6, 6, 2, 6 );
  
  crude_vector Q1 = crude_vec_multiply(P1, Sign );
  crude_vector Q0 = crude_vec_multiply(P0, Y0);
  Q1 = crude_vec_multiply(Q1, Y1);
  Q0 = crude_vec_multiply(Q0, R0);
  crude_vector Q = crude_vec_multiply_add(Q1, R1, Q0);
  
  return Q;
}

/************************************************
 *
 * Matrix functions
 * 
 ***********************************************/
crude_matrix
crude_mat_perspective_fov_lh
(
  _In_ float32 fov_angle_y,
  _In_ float32 aspect_ratio,
  _In_ float32 nearz,
  _In_ float32 farz
)
{
  float SinFov;
  float CosFov;
  crude_sin_cos( &SinFov, &CosFov, 0.5f * fov_angle_y );
  
  float fRange = farz / ( farz - nearz );
  // Note: This is recorded on the stack
  float Height = CosFov / SinFov;
  crude_vector rMem = {
    Height / aspect_ratio,
    Height,
    fRange,
    -fRange * nearz
  };
  // Copy from memory to SSE register
  crude_vector vValues = rMem;
  crude_vector vTemp = _mm_setzero_ps();
  // Copy x only
  vTemp = _mm_move_ss(vTemp, vValues);
  // Height / AspectRatio,0,0,0
  crude_matrix M;
  M.r[0] = vTemp;
  // 0,Height,0,0
  vTemp = vValues;
  vTemp = _mm_and_ps(vTemp, CRUDE_MATH_MASK_Y);
  M.r[1] = vTemp;
  // x=fRange,y=-fRange * NearZ,0,1.0f
  vTemp = _mm_setzero_ps();
  vValues = _mm_shuffle_ps(vValues, CRUDE_MATH_IDENTITY_R3, _MM_SHUFFLE(3, 2, 3, 2));
  // 0,0,fRange,1.0f
  vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(3, 0, 0, 0));
  M.r[2] = vTemp;
  // 0,0,-fRange * NearZ,0.0f
  vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(2, 1, 0, 0));
  M.r[3] = vTemp;
  return M;
}

crude_matrix
crude_mat_multiply
(
  _In_ crude_matrix                              m1,
  _In_ crude_matrix                              m2
)
{
  // copy from inline XMMATRIX XM_CALLCONV XMMatrixMultiply
  __m256 t0 = _mm256_castps128_ps256(m1.r[0]);
  t0 = _mm256_insertf128_ps(t0, m1.r[1], 1);
  __m256 t1 = _mm256_castps128_ps256(m1.r[2]);
  t1 = _mm256_insertf128_ps(t1, m1.r[3], 1);
  
  __m256 u0 = _mm256_castps128_ps256(m2.r[0]);
  u0 = _mm256_insertf128_ps(u0, m2.r[1], 1);
  __m256 u1 = _mm256_castps128_ps256(m2.r[2]);
  u1 = _mm256_insertf128_ps(u1, m2.r[3], 1);
  
  __m256 a0 = _mm256_shuffle_ps(t0, t0, _MM_SHUFFLE(0, 0, 0, 0));
  __m256 a1 = _mm256_shuffle_ps(t1, t1, _MM_SHUFFLE(0, 0, 0, 0));
  __m256 b0 = _mm256_permute2f128_ps(u0, u0, 0x00);
  __m256 c0 = _mm256_mul_ps(a0, b0);
  __m256 c1 = _mm256_mul_ps(a1, b0);
  
  a0 = _mm256_shuffle_ps(t0, t0, _MM_SHUFFLE(1, 1, 1, 1));
  a1 = _mm256_shuffle_ps(t1, t1, _MM_SHUFFLE(1, 1, 1, 1));
  b0 = _mm256_permute2f128_ps(u0, u0, 0x11);
  __m256 c2 = _mm256_fmadd_ps(a0, b0, c0);
  __m256 c3 = _mm256_fmadd_ps(a1, b0, c1);
  
  a0 = _mm256_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 2, 2, 2));
  a1 = _mm256_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 2, 2, 2));
  __m256 b1 = _mm256_permute2f128_ps(u1, u1, 0x00);
  __m256 c4 = _mm256_mul_ps(a0, b1);
  __m256 c5 = _mm256_mul_ps(a1, b1);
  
  a0 = _mm256_shuffle_ps(t0, t0, _MM_SHUFFLE(3, 3, 3, 3));
  a1 = _mm256_shuffle_ps(t1, t1, _MM_SHUFFLE(3, 3, 3, 3));
  b1 = _mm256_permute2f128_ps(u1, u1, 0x11);
  __m256 c6 = _mm256_fmadd_ps(a0, b1, c4);
  __m256 c7 = _mm256_fmadd_ps(a1, b1, c5);
  
  t0 = _mm256_add_ps(c2, c6);
  t1 = _mm256_add_ps(c3, c7);
  
  crude_matrix mResult;
  mResult.r[0] = _mm256_castps256_ps128(t0);
  mResult.r[1] = _mm256_extractf128_ps(t0, 1);
  mResult.r[2] = _mm256_castps256_ps128(t1);
  mResult.r[3] = _mm256_extractf128_ps(t1, 1);
  return mResult;
}

crude_matrix
crude_mat_affine_transformation
(
  _In_ crude_vector                              scaling,
  _In_ crude_vector                              rotation_origin,
  _In_ crude_vector                              rotation_quaternion,
  _In_ crude_vector                              translation
)
{
  crude_matrix mscaling = crude_mat_scaling_from_vector( scaling );
  crude_vector vrotation_origin = crude_vec_select( CRUDE_MATH_SELECT_1110, rotation_origin, CRUDE_MATH_SELECT_1110 );
  crude_matrix mrotation = crude_mat_rotation_quaternion( rotation_quaternion );
  crude_vector vtranslation = crude_vec_select( CRUDE_MATH_SELECT_1110, translation, CRUDE_MATH_SELECT_1110 );
  
  crude_matrix m;
  m = mscaling;
  m.r[3] = crude_vec_subtract( m.r[3], vrotation_origin );
  m = crude_mat_multiply( m, mrotation );
  m.r[3] = crude_vec_add( m.r[3], vrotation_origin );
  m.r[3] = crude_vec_add( m.r[3], vtranslation );
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

  v_temp = _mm_div_ps( CRUDE_MATH_VECTOR_ONE, v_temp );
  
  crude_matrix mResult;
  mResult.r[ 0 ] = _mm_mul_ps( c0, v_temp );
  mResult.r[ 1 ] = _mm_mul_ps( c2, v_temp );
  mResult.r[ 2 ] = _mm_mul_ps( c4, v_temp );
  mResult.r[ 3 ] = _mm_mul_ps( c6, v_temp );
  return mResult;
}

crude_matrix
crude_mat_scaling_from_vector
(
  _In_ crude_vector const                        scale
)
{
  crude_matrix m;
  m.r[0] = _mm_and_ps( scale, CRUDE_MATH_MASK_X );
  m.r[1] = _mm_and_ps( scale , CRUDE_MATH_MASK_Y );
  m.r[2] = _mm_and_ps( scale, CRUDE_MATH_MASK_Z );
  m.r[3] = CRUDE_MATH_IDENTITY_R3;
  return m;
}

crude_matrix
crude_mat_rotation_quaternion
(
  _In_ crude_vector const                        quaternion
)
{
  crude_vector Q0 = _mm_add_ps(quaternion, quaternion);
  crude_vector Q1 = _mm_mul_ps(quaternion, Q0);
  
  crude_vector V0 = _mm_permute_ps(Q1, _MM_SHUFFLE(3, 0, 0, 1));
  V0 = _mm_and_ps(V0, CRUDE_MATH_MASK_3 );
  crude_vector V1 = _mm_permute_ps(Q1, _MM_SHUFFLE(3, 1, 2, 2));
  V1 = _mm_and_ps(V1, CRUDE_MATH_MASK_3 );
  crude_vector R0 = _mm_sub_ps( _mm_set_ps( 0.0f, 1.0f, 1.0f, 1.0f ), V0);
  R0 = _mm_sub_ps(R0, V1);
  
  V0 = _mm_permute_ps(quaternion, _MM_SHUFFLE(3, 1, 0, 0));
  V1 = _mm_permute_ps(Q0, _MM_SHUFFLE(3, 2, 1, 2));
  V0 = _mm_mul_ps(V0, V1);
  
  V1 = _mm_permute_ps(quaternion, _MM_SHUFFLE(3, 3, 3, 3));
  crude_vector V2 = _mm_permute_ps(Q0, _MM_SHUFFLE(3, 0, 2, 1));
  V1 = _mm_mul_ps(V1, V2);
  
  crude_vector R1 = _mm_add_ps(V0, V1);
  crude_vector R2 = _mm_sub_ps(V0, V1);
  
  V0 = _mm_shuffle_ps(R1, R2, _MM_SHUFFLE(1, 0, 2, 1));
  V0 = _mm_permute_ps(V0, _MM_SHUFFLE(1, 3, 2, 0));
  V1 = _mm_shuffle_ps(R1, R2, _MM_SHUFFLE(2, 2, 0, 0));
  V1 = _mm_permute_ps(V1, _MM_SHUFFLE(2, 0, 2, 0));
  
  Q1 = _mm_shuffle_ps(R0, V0, _MM_SHUFFLE(1, 0, 3, 0));
  Q1 = _mm_permute_ps(Q1, _MM_SHUFFLE(1, 3, 2, 0));
  
  crude_matrix M;
  M.r[0] = Q1;
  
  Q1 = _mm_shuffle_ps(R0, V0, _MM_SHUFFLE(3, 2, 3, 1));
  Q1 = _mm_permute_ps(Q1, _MM_SHUFFLE(1, 3, 0, 2));
  M.r[1] = Q1;
  
  Q1 = _mm_shuffle_ps(V1, R0, _MM_SHUFFLE(3, 2, 1, 0));
  M.r[2] = Q1;
  M.r[3] = CRUDE_MATH_IDENTITY_R3;
  return M;
}

/************************************************
 *
 * Function to convert SIMD/Structs
 * 
 ***********************************************/
void
crude_store_float3
(
  _Out_ crude_float3                            *f,
  _In_ crude_vector                              v
)
{
  *( int* )( &f->x ) = _mm_extract_ps( v, 0 );
  *( int* )( &f->y ) = _mm_extract_ps( v, 1 );
  *( int* )( &f->z ) = _mm_extract_ps( v, 2 );
}

void
crude_store_float4
(
  _Out_ crude_float4                            *f,
  _In_ crude_vector                              v
)
{
  _mm_storeu_ps( &f->x, v );
}

void
crude_store_float4x4
(
  _Out_ crude_float4x4                          *f,
  _In_ crude_matrix const                        m
)
{
  _mm_storeu_ps( &f->_00, m.r[ 0 ] );
  _mm_storeu_ps( &f->_10, m.r[ 1 ] );
  _mm_storeu_ps( &f->_20, m.r[ 2 ] );
  _mm_storeu_ps( &f->_30, m.r[ 3 ] );
}

void
crude_store_float4x4a
(
  _Out_ crude_float4x4a                         *f,
  _In_ crude_matrix const                        m
)
{
  _mm_store_ps( &f->_00, m.r[ 0 ] );
  _mm_store_ps( &f->_10, m.r[ 1 ] );
  _mm_store_ps( &f->_20, m.r[ 2 ] );
  _mm_store_ps( &f->_30, m.r[ 3 ] );
}

crude_vector
crude_load_float3
(
  _In_ crude_float3 const                       *f
)
{
  __m128 xy = _mm_castpd_ps( _mm_load_sd( ( const double* )( f ) ) );
  __m128 z = _mm_load_ss( &f->z );
  return _mm_insert_ps( xy, z, 0x20 );
}

crude_vector
crude_load_float4
(
  _In_ crude_float4 const                       *f
)
{
  return _mm_loadu_ps( &f->x );
}