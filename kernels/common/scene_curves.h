// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "default.h"
#include "geometry.h"
#include "buffer.h"

namespace embree
{
  /*! represents an array of bicubic bezier curves */
  struct CurveGeometry : public Geometry
  {
    /*! type of this geometry */
    static const Geometry::GTypeMask geom_type = Geometry::MTY_CURVE4;

  public:
    
    /*! bezier curve construction */
    CurveGeometry (Device* device, Geometry::GType gtype);
    
  public:
    void enabling();
    void disabling();
    void setMask(unsigned mask);
    void setNumTimeSteps (unsigned int numTimeSteps);
    void setVertexAttributeCount (unsigned int N);
    void setBuffer(RTCBufferType type, unsigned int slot, RTCFormat format, const Ref<Buffer>& buffer, size_t offset, size_t stride, unsigned int num);
    void* getBuffer(RTCBufferType type, unsigned int slot);
    void updateBuffer(RTCBufferType type, unsigned int slot);
    void preCommit();
    void postCommit();
    bool verify();
    void setTessellationRate(float N);

  public:
    
    /*! returns the number of vertices */
    __forceinline size_t numVertices() const {
      return vertices[0].size();
    }

    /*! returns the i'th curve */
    __forceinline const unsigned int& curve(size_t i) const {
      return curves[i];
    }

    /*! returns the i'th segment */
    __forceinline unsigned int getStartEndBitMask(size_t i) const {
      unsigned int mask = 0;
      if (flags) 
        mask |= (flags[i] & 0x3) << 30;
      return mask;
    }

    /*! returns i'th vertex of the first time step */
    __forceinline Vec3fa vertex(size_t i) const {
      return vertices0[i];
    }

    /*! returns i'th normal of the first time step */
    __forceinline Vec3fa normal(size_t i) const {
      return normals0[i];
    }

    /*! returns i'th tangent of the first time step */
    __forceinline Vec3fa tangent(size_t i) const {
      return tangents0[i];
    }

    /*! returns i'th radius of the first time step */
    __forceinline float radius(size_t i) const {
      return vertices0[i].w;
    }

    /*! returns i'th vertex of itime'th timestep */
    __forceinline Vec3fa vertex(size_t i, size_t itime) const {
      return vertices[itime][i];
    }

    /*! returns i'th normal of itime'th timestep */
    __forceinline Vec3fa normal(size_t i, size_t itime) const {
      return normals[itime][i];
    }

    /*! returns i'th tangent of itime'th timestep */
    __forceinline Vec3fa tangent(size_t i, size_t itime) const {
      return tangents[itime][i];
    }

    /*! returns i'th radius of itime'th timestep */
    __forceinline float radius(size_t i, size_t itime) const {
      return vertices[itime][i].w;
    }

    /*! gathers the curve starting with i'th vertex */
    __forceinline void gather(Vec3fa& p0, Vec3fa& p1, Vec3fa& p2, Vec3fa& p3, size_t i) const
    {
      p0 = vertex(i+0);
      p1 = vertex(i+1);
      p2 = vertex(i+2);
      p3 = vertex(i+3);
    }

    /*! gathers the curve starting with i'th vertex of itime'th timestep */
    __forceinline void gather(Vec3fa& p0, Vec3fa& p1, Vec3fa& p2, Vec3fa& p3, size_t i, size_t itime) const
    {
      p0 = vertex(i+0,itime);
      p1 = vertex(i+1,itime);
      p2 = vertex(i+2,itime);
      p3 = vertex(i+3,itime);
    }

    /*! gathers the curve starting with i'th vertex */
    __forceinline void gather(Vec3fa& p0, Vec3fa& p1, Vec3fa& p2, Vec3fa& p3, Vec3fa& n0, Vec3fa& n1, size_t i) const
    {
      p0 = vertex(i+0);
      p1 = vertex(i+1);
      p2 = vertex(i+2);
      p3 = vertex(i+3);
      n0 = normal(i+0);
      n1 = normal(i+1);
    }

    /*! gathers the curve starting with i'th vertex of itime'th timestep */
    __forceinline void gather(Vec3fa& p0, Vec3fa& p1, Vec3fa& p2, Vec3fa& p3, Vec3fa& n0, Vec3fa& n1, size_t i, size_t itime) const
    {
      p0 = vertex(i+0,itime);
      p1 = vertex(i+1,itime);
      p2 = vertex(i+2,itime);
      p3 = vertex(i+3,itime);
      n0 = normal(i+0,itime);
      n1 = normal(i+1,itime);
    }

    /*! prefetches the curve starting with i'th vertex of itime'th timestep */
    __forceinline void prefetchL1_vertices(size_t i) const
    {
      prefetchL1(vertices0.getPtr(i)+0);
      prefetchL1(vertices0.getPtr(i)+64);
    }

    /*! prefetches the curve starting with i'th vertex of itime'th timestep */
    __forceinline void prefetchL2_vertices(size_t i) const
    {
      prefetchL2(vertices0.getPtr(i)+0);
      prefetchL2(vertices0.getPtr(i)+64);
    }  

    /*! loads curve vertices for specified time */
    __forceinline void gather(Vec3fa& p0, Vec3fa& p1, Vec3fa& p2, Vec3fa& p3, size_t i, float time) const
    {
      float ftime;
      const size_t itime = getTimeSegment(time, fnumTimeSegments, ftime);

      const float t0 = 1.0f - ftime;
      const float t1 = ftime;
      Vec3fa a0,a1,a2,a3;
      gather(a0,a1,a2,a3,i,itime);
      Vec3fa b0,b1,b2,b3;
      gather(b0,b1,b2,b3,i,itime+1);
      p0 = madd(Vec3fa(t0),a0,t1*b0);
      p1 = madd(Vec3fa(t0),a1,t1*b1);
      p2 = madd(Vec3fa(t0),a2,t1*b2);
      p3 = madd(Vec3fa(t0),a3,t1*b3);
    }

    /*! loads curve vertices for specified time */
    __forceinline void gather(Vec3fa& p0, Vec3fa& p1, Vec3fa& p2, Vec3fa& p3, Vec3fa& n0, Vec3fa& n1, size_t i, float time) const
    {
      float ftime;
      const size_t itime = getTimeSegment(time, fnumTimeSegments, ftime);

      const float t0 = 1.0f - ftime;
      const float t1 = ftime;
      Vec3fa a0,a1,a2,a3,an0,an1;
      gather(a0,a1,a2,a3,an0,an1,i,itime);
      Vec3fa b0,b1,b2,b3,bn0,bn1;
      gather(b0,b1,b2,b3,bn0,bn1,i,itime+1);
      p0 = madd(Vec3fa(t0),a0,t1*b0);
      p1 = madd(Vec3fa(t0),a1,t1*b1);
      p2 = madd(Vec3fa(t0),a2,t1*b2);
      p3 = madd(Vec3fa(t0),a3,t1*b3);
      n0 = madd(Vec3fa(t0),an0,t1*bn0);
      n1 = madd(Vec3fa(t0),an1,t1*bn1);
    }

    /*! gathers the hermite curve starting with i'th vertex */
    __forceinline void gather_hermite(Vec3fa& p0, Vec3fa& t0, Vec3fa& p1, Vec3fa& t1, size_t i) const
    {
      p0 = vertex (i+0);
      p1 = vertex (i+1);
      t0 = tangent(i+0);
      t1 = tangent(i+1);
    }

    /*! gathers the hermite curve starting with i'th vertex of itime'th timestep */
    __forceinline void gather_hermite(Vec3fa& p0, Vec3fa& t0, Vec3fa& p1, Vec3fa& t1, size_t i, size_t itime) const
    {
      p0 = vertex (i+0,itime);
      p1 = vertex (i+1,itime);
      t0 = tangent(i+0,itime);
      t1 = tangent(i+1,itime);
    }

    /*! loads curve vertices for specified time */
    __forceinline void gather_hermite(Vec3fa& p0, Vec3fa& t0, Vec3fa& p1, Vec3fa& t1, size_t i, float time) const
    {
      float ftime;
      const size_t itime = getTimeSegment(time, fnumTimeSegments, ftime);
      const float f0 = 1.0f - ftime, f1 = ftime;
      Vec3fa ap0,at0,ap1,at1;
      gather_hermite(ap0,at0,ap1,at1,i,itime);
      Vec3fa bp0,bt0,bp1,bt1;
      gather_hermite(bp0,bt0,bp1,bt1,i,itime+1);
      p0 = madd(Vec3fa(f0),ap0,f1*bp0);
      t0 = madd(Vec3fa(f0),at0,f1*bt0);
      p1 = madd(Vec3fa(f0),ap1,f1*bp1);
      t1 = madd(Vec3fa(f0),at1,f1*bt1);
    }

    /*! gathers the hermite curve starting with i'th vertex */
    __forceinline void gather_hermite(Vec3fa& p0, Vec3fa& t0, Vec3fa& n0, Vec3fa& p1, Vec3fa& t1, Vec3fa& n1, size_t i) const
    {
      p0 = vertex (i+0);
      p1 = vertex (i+1);
      t0 = tangent(i+0);
      t1 = tangent(i+1);
      n0 = normal(i+0);
      n1 = normal(i+1);
    }

    /*! gathers the hermite curve starting with i'th vertex of itime'th timestep */
    __forceinline void gather_hermite(Vec3fa& p0, Vec3fa& t0, Vec3fa& n0, Vec3fa& p1, Vec3fa& t1, Vec3fa& n1, size_t i, size_t itime) const
    {
      p0 = vertex (i+0,itime);
      p1 = vertex (i+1,itime);
      t0 = tangent(i+0,itime);
      t1 = tangent(i+1,itime);
      n0 = normal(i+0,itime);
      n1 = normal(i+1,itime);
    }

    /*! loads curve vertices for specified time */
    __forceinline void gather_hermite(Vec3fa& p0, Vec3fa& t0, Vec3fa& n0, Vec3fa& p1, Vec3fa& t1, Vec3fa& n1, size_t i, float time) const
    {
      float ftime;
      const size_t itime = getTimeSegment(time, fnumTimeSegments, ftime);
      const float f0 = 1.0f - ftime, f1 = ftime;
      Vec3fa ap0,at0,an0,ap1,at1,an1;
      gather_hermite(ap0,at0,an0,ap1,at1,an1,i,itime);
      Vec3fa bp0,bt0,bn0,bp1,bt1,bn1;
      gather_hermite(bp0,bt0,bn0,bp1,bt1,bn1,i,itime+1);
      p0 = madd(Vec3fa(f0),ap0,f1*bp0);
      t0 = madd(Vec3fa(f0),at0,f1*bt0);
      n0 = madd(Vec3fa(f0),an0,f1*bn0);
      p1 = madd(Vec3fa(f0),ap1,f1*bp1);
      t1 = madd(Vec3fa(f0),at1,f1*bt1);
      n1 = madd(Vec3fa(f0),an1,f1*bn1);
    }

  public:
    BufferView<unsigned int> curves;        //!< array of curve indices
    BufferView<Vec3fa> vertices0;           //!< fast access to first vertex buffer
    BufferView<Vec3fa> normals0;            //!< fast access to first normal buffer
    BufferView<Vec3fa> tangents0;           //!< fast access to first tangent buffer
    vector<BufferView<Vec3fa>> vertices;    //!< vertex array for each timestep
    vector<BufferView<Vec3fa>> normals;     //!< normal array for each timestep
    vector<BufferView<Vec3fa>> tangents;    //!< tangent array for each timestep
    BufferView<char> flags;                 //!< start, end flag per segment
    vector<BufferView<char>> vertexAttribs; //!< user buffers
    int tessellationRate;                   //!< tessellation rate for bezier curve
  };
  
  DECLARE_ISA_FUNCTION(CurveGeometry*, createCurves, Device* COMMA Geometry::GType);
}
