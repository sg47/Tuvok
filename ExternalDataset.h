/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2009 Scientific Computing and Imaging Institute,
   University of Utah.


   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

/**
  \author  Tom Fogal
           SCI Institute
           University of Utah
*/
#pragma once

#ifndef TUVOK_EXTERNAL_DATASET_H
#define TUVOK_EXTERNAL_DATASET_H

#include <StdTuvokDefines.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include "BrickedDataset.h"
#include "VariantArray.h"
#include "Controller/Controller.h"

namespace tuvok {

/** An external dataset is a bricked dataset which comes from a source
 * which is not under Tuvok's control.  Many existing applications generate
 * and store data their own way, and simply want Tuvok to render it.  This
 * provides a mechanism for Tuvok to share ownership with the client
 * applications' data. */
class ExternalDataset : public BrickedDataset {
public:
  ExternalDataset();
  virtual ~ExternalDataset();

  virtual float MaxGradientMagnitude() const;

  virtual UINTVECTOR3 GetBrickVoxelCounts(const BrickKey&) const;
  virtual bool GetBrick(const BrickKey&, std::vector<uint8_t>&) const;
  virtual bool GetBrick(const BrickKey&, std::vector<int8_t>&) const;
  virtual bool GetBrick(const BrickKey&, std::vector<uint16_t>&) const;
  virtual bool GetBrick(const BrickKey&, std::vector<int16_t>&) const;
  virtual bool GetBrick(const BrickKey&, std::vector<uint32_t>&) const;
  virtual bool GetBrick(const BrickKey&, std::vector<int32_t>&) const;
  virtual bool GetBrick(const BrickKey&, std::vector<float>&) const;
  virtual bool GetBrick(const BrickKey&, std::vector<double>&) const;

  virtual UINTVECTOR3 GetBrickOverlapSize() const {
    // Hack; should have a setter for this, and query the info it sets here.
    return UINTVECTOR3(1,1,1);
  }

  virtual UINT64VECTOR3 GetEffectiveBrickSize(const BrickKey& bk) const {
    const UINTVECTOR3& voxels = this->GetBrickMetadata(bk).n_voxels;
    /// @todo FIXME: WRONG -- assumes internal brick.
    /// we need a setter for this info, and then query the info it sets here.
    return UINT64VECTOR3(voxels[0]-1, voxels[1]-1, voxels[2]-1);
  }

  virtual uint64_t GetLODLevelCount() const { /* hack! */ return 1; }

  /// Uploads external histograms.  One will be calculated implicitly if not
  /// given.  To prevent this (potentially extra) work, upload the histogram
  /// *before* the data.
  ///@{
  void SetHistogram(const std::vector<uint32_t>&);
  void SetHistogram(const std::vector<std::vector<uint32_t> >&);
  ///@}

  /// Adds a brick to the dataset.  The first two parameters give the data
  /// itself.  The next three give the dimension of the brick: the number of
  /// data points in each dimension.  Data are assumed to be centered on the
  /// nodes of a cartesian grid.  Finally, the last three parameters give the
  /// index of this brick in the overall dataset, with (0,0,0) being the
  /// leftmost, bottom, closest brick, and (NX, NY, NZ) being the rightmost,
  /// topmost, farthest brick.
  ///@{
  void AddBrick(const BrickKey&, const BrickMD&,
                const std::shared_ptr<double>, size_t len,
                double fMin, double fMax);
  void AddBrick(const BrickKey&, const BrickMD&,
                const std::shared_ptr<float>, size_t len,
                float fMin, float fMax);
  void AddBrick(const BrickKey&, const BrickMD&,
                const std::shared_ptr<unsigned char>, size_t len,
                unsigned char ubMin, unsigned char ubMax);
  void AddBrick(const BrickKey&, const BrickMD&,
                const std::shared_ptr<short>, size_t len,
                short sMin, short sMax);
  void AddBrick(const BrickKey&, const BrickMD&,
                const std::shared_ptr<unsigned short>, size_t len,
                unsigned short sMin, unsigned short sMax);
  ///@}
  ///@}
  /// Updates the data within an existing brick.
  ///@{
  void UpdateData(const BrickKey&, const std::shared_ptr<double>,
                  size_t len);
  void UpdateData(const BrickKey&, const std::shared_ptr<float>,
                  size_t len);
  void UpdateData(const BrickKey&, const std::shared_ptr<unsigned char>,
                  size_t len);
  void UpdateData(const BrickKey&, const std::shared_ptr<short>,
                  size_t len);
  void UpdateData(const BrickKey&, const std::shared_ptr<unsigned short>,
                  size_t len);
  ///@}
  void Clear();

  /// Thrown when a brick lookup fails.
  struct BrickNotFound : public std::exception {
    BrickNotFound(const char *m) : msg(m) {}
    virtual const char *what() const throw() { return msg; }
    private: const char *msg;
  };

  /// Important for correct 2D transfer function rendering.
  void SetGradientMagnitudeRange(float, float);

  /// Number of bits in the data representation.
  virtual uint64_t GetBitWidth() const {
    // We query VariantArray for the type.  Yet we have one variant array per
    // brick; theoretically each brick could have a different underlying data
    // type.  However our model doesn't handle that: the data type is global
    // for a dataset.  So we simply query the first brick && use its data type
    // to represent the entire dataset's data type.
    assert(!this->m_Data.empty());
    DataTable::const_iterator iter = this->m_Data.begin();
    switch(iter->second.type()) {
      case VariantArray::DT_UBYTE:  return  8;
      case VariantArray::DT_BYTE:   return  8;
      case VariantArray::DT_USHORT: return 16;
      case VariantArray::DT_SHORT:  return 16;
      case VariantArray::DT_FLOAT:  return 32;
      case VariantArray::DT_DOUBLE: return 64;
    }
    assert(1==0);
    return 42;
  }
  /// Number of components per data point.
  uint64_t GetComponentCount() const {
    WARNING("Assuming single-component data.");
    return 1;
  }
  bool GetIsSigned() const {
    assert(!this->m_Data.empty());
    DataTable::const_iterator iter = this->m_Data.begin();
    switch(iter->second.type()) {
      case VariantArray::DT_UBYTE:  return false;
      case VariantArray::DT_BYTE:   return true;
      case VariantArray::DT_USHORT: return false;
      case VariantArray::DT_SHORT:  return true;
      case VariantArray::DT_FLOAT:  return true;
      case VariantArray::DT_DOUBLE: return true;
    }
    return true;
  }
  bool GetIsFloat() const {
    assert(!this->m_Data.empty());
    DataTable::const_iterator iter = this->m_Data.begin();
    return iter->second.type() == VariantArray::DT_FLOAT ||
           iter->second.type() == VariantArray::DT_DOUBLE;
  }
  bool IsSameEndianness() const {
    return true;
  }
  UINT64VECTOR3 GetDomainSize(const size_t=0,
                              const size_t=0) const {
    return m_vDomainSize;
  }
  void SetDomainSize(uint64_t x, uint64_t y, uint64_t z) {
    m_vDomainSize = UINT64VECTOR3(x,y,z);
  }
  void SetRange(const std::pair<double, double>&);
  void SetRange(double low, double high) {
    this->SetRange(std::make_pair(low, high));
  }
  virtual std::pair<double,double> GetRange() const;

  /// unimplemented!  Not really needed for integrations.
  virtual bool Export(uint64_t, const std::string&, bool) const {
    return false;
  }
  /// unimplemented!  Not really needed for integrations.
  virtual bool ApplyFunction(
    uint64_t,
    bool (*)(void* pData, const UINT64VECTOR3& vBrickSize,
             const UINT64VECTOR3& vBrickOffset,
             void* pUserContext),
    void *,
    uint64_t
  ) const { return false; }

  virtual const char* Name() const { return "External dataset"; }
  virtual ExternalDataset* Create(const std::string&, uint64_t, bool) const;

  typedef std::unordered_map<BrickKey, VariantArray, BKeyHash> DataTable;

protected:
  /// Should the data change and the client isn't going to supply a histogram,
  /// we should supply one ourself.
  void Recalculate1DHistogram();

  DataTable::const_iterator Lookup(const BrickKey& k) const;

private:
  DataTable                m_Data;
  float                    m_fMaxMagnitude;
  UINT64VECTOR3            m_vDomainSize;
  std::pair<double,double> m_DataRange;
};

}; // namespace tuvok

#endif // TUVOK_EXTERNAL_DATASET_H
