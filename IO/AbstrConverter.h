/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
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
  \file    AbstrConverter.h
  \author  Jens Krueger
           SCI Institute
           University of Utah
  \version 1.0
  \date    December 2008
*/


#pragma once

#ifndef ABSTRCONVERTER_H
#define ABSTRCONVERTER_H

#include "../StdTuvokDefines.h"
#include "../IO/UVF/UVF.h"
#include "../Basics/Vectors.h"

class MasterController;

class AbstrConverter {
public:
  virtual ~AbstrConverter() {}

  virtual bool Convert(const std::string& strSourceFilename, const std::string& strTargetFilename, const std::string& strTempDir, MasterController* pMasterController, bool bNoUserInteraction) = 0;
  const std::vector<std::string>& SupportedExt() {return m_vSupportedExt;}
  virtual const std::string& GetDesc() {return m_vConverterDesc;}

protected:
  std::string               m_vConverterDesc;
  std::vector<std::string>  m_vSupportedExt;
  
  static const std::string Process8BitsTo8Bits(UINT64 iHeaderSkip, const std::string& strFilename, const std::string& strTargetFilename, size_t iSize, bool bSigned, Histogram1DDataBlock& Histogram1D, MasterController* m_pMasterController);
  static const std::string QuantizeShortTo12Bits(UINT64 iHeaderSkip, const std::string& strFilename, const std::string& strTargetFilename, size_t iSize, bool bSigned, Histogram1DDataBlock& Histogram1D, MasterController* m_pMasterController);
  static const std::string QuantizeFloatTo12Bits(UINT64 iHeaderSkip, const std::string& strFilename, const std::string& strTargetFilename, size_t iSize, Histogram1DDataBlock& Histogram1D, MasterController* m_pMasterController);
};

#endif // ABSTRCONVERTER_H