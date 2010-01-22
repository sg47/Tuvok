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
  \file    GLSBVR2D.cpp
  \author  Jens Krueger
           SCI Institute
           University of Utah
  \date    August 2008
*/
#include "GLInclude.h"
#include "GLSBVR2D.h"

#include "Controller/Controller.h"
#include "Renderer/GL/GLSLProgram.h"
#include "Renderer/GL/GLTexture1D.h"
#include "Renderer/GL/GLTexture2D.h"
#include "Renderer/GPUMemMan/GPUMemMan.h"
#include "Renderer/TFScaling.h"

using namespace std;
using namespace tuvok;

GLSBVR2D::GLSBVR2D(MasterController* pMasterController, bool bUseOnlyPowerOfTwo, bool bDownSampleTo8Bits, bool bDisableBorder) :
  GLRenderer(pMasterController, bUseOnlyPowerOfTwo, bDownSampleTo8Bits, bDisableBorder),
  m_pProgramIsoNoCompose(NULL),
  m_pProgramColorNoCompose(NULL)
{
}

GLSBVR2D::~GLSBVR2D() {
}

void GLSBVR2D::Cleanup() {
  GLRenderer::Cleanup();
  if (m_pProgramIsoNoCompose)   {m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramIsoNoCompose); m_pProgramIsoNoCompose =NULL;}
  if (m_pProgramColorNoCompose) {m_pMasterController->MemMan()->FreeGLSLProgram(m_pProgramColorNoCompose); m_pProgramColorNoCompose =NULL;}
}

bool GLSBVR2D::Initialize() {
  if (!GLRenderer::Initialize()) {
    T_ERROR("Error in parent call -> aborting");
    return false;
  }

  if (!LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-1D-FS.glsl",       m_vShaderSearchDirs, &(m_pProgram1DTrans[0])) ||
      !LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-1D-light-FS.glsl", m_vShaderSearchDirs, &(m_pProgram1DTrans[1])) ||
      !LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-2D-FS.glsl",       m_vShaderSearchDirs, &(m_pProgram2DTrans[0])) ||
      !LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-2D-light-FS.glsl", m_vShaderSearchDirs, &(m_pProgram2DTrans[1])) ||
      !LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-MIP-Rot-FS.glsl",  m_vShaderSearchDirs, &(m_pProgramHQMIPRot)) ||
      !LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-ISO-FS.glsl",      m_vShaderSearchDirs, &(m_pProgramIso)) ||
      !LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-Color-FS.glsl",    m_vShaderSearchDirs, &(m_pProgramColor)) ||
      !LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-ISO-NC-FS.glsl",   m_vShaderSearchDirs, &(m_pProgramIsoNoCompose)) ||
      !LoadAndVerifyShader("GLSBVR-VS.glsl", "GLSBVR-Color-NC-FS.glsl", m_vShaderSearchDirs, &(m_pProgramColorNoCompose))) {

      Cleanup();

      T_ERROR("Error loading a shader.");
      return false;
  } else {
    m_pProgram1DTrans[0]->Enable();
    m_pProgram1DTrans[0]->SetUniformVector("texVolume",0);
    m_pProgram1DTrans[0]->SetUniformVector("texTrans1D",1);
    m_pProgram1DTrans[0]->Disable();

    m_pProgram1DTrans[1]->Enable();
    m_pProgram1DTrans[1]->SetUniformVector("texVolume",0);
    m_pProgram1DTrans[1]->SetUniformVector("texTrans1D",1);
    m_pProgram1DTrans[1]->Disable();

    m_pProgram2DTrans[0]->Enable();
    m_pProgram2DTrans[0]->SetUniformVector("texVolume",0);
    m_pProgram2DTrans[0]->SetUniformVector("texTrans2D",1);
    m_pProgram2DTrans[0]->Disable();

    m_pProgram2DTrans[1]->Enable();
    m_pProgram2DTrans[1]->SetUniformVector("texVolume",0);
    m_pProgram2DTrans[1]->SetUniformVector("texTrans2D",1);
    m_pProgram2DTrans[1]->Disable();

    m_pProgramIso->Enable();
    m_pProgramIso->SetUniformVector("texVolume",0);
    m_pProgramIso->Disable();

    m_pProgramColor->Enable();
    m_pProgramColor->SetUniformVector("texVolume",0);
    m_pProgramColor->Disable();

    m_pProgramHQMIPRot->Enable();
    m_pProgramHQMIPRot->SetUniformVector("texVolume",0);
    m_pProgramHQMIPRot->Disable();

    m_pProgramIsoNoCompose->Enable();
    m_pProgramIsoNoCompose->SetUniformVector("texVolume",0);
    m_pProgramIsoNoCompose->Disable();

    m_pProgramColorNoCompose->Enable();
    m_pProgramColorNoCompose->SetUniformVector("texVolume",0);
    m_pProgramColorNoCompose->Disable();

    UpdateColorsInShaders();

  }

  return true;
}

void GLSBVR2D::SetDataDepShaderVars() {
  GLRenderer::SetDataDepShaderVars();

  if (m_eRenderMode == RM_ISOSURFACE && m_bAvoidSeperateCompositing) {
    GLSLProgram* shader = (m_pDataset->GetComponentCount() == 1) ? m_pProgramIsoNoCompose : m_pProgramColorNoCompose;

    FLOATVECTOR3 d = m_cDiffuse.xyz()*m_cDiffuse.w;

    shader->Enable();
    shader->SetUniformVector("fIsoval", static_cast<float>
                                        (this->GetNormalizedIsovalue()));
    // this is not really a data dependent var but as we only need to
    // do it once per frame we may also do it here
    shader->SetUniformVector("vLightDiffuse",d.x*m_vIsoColor.x,d.y*m_vIsoColor.y,d.z*m_vIsoColor.z);
    shader->Disable();
  }

  if(m_eRenderMode == RM_1DTRANS && m_TFScalingMethod == SMETH_BIAS_AND_SCALE) {
    std::pair<float,float> bias_scale = scale_bias_and_scale(*m_pDataset);
    MESSAGE("setting TF bias (%5.3f) and scale (%5.3f)", bias_scale.first,
            bias_scale.second);
    m_pProgram1DTrans[0]->Enable();
    m_pProgram1DTrans[0]->SetUniformVector("TFuncBias", bias_scale.first);
    m_pProgram1DTrans[0]->SetUniformVector("fTransScale", bias_scale.second);
    m_pProgram1DTrans[0]->Disable();
  }
}

void GLSBVR2D::SetBrickDepShaderVars(const Brick& currentBrick) {
  FLOATVECTOR3 vStep(1.0f/currentBrick.vVoxelCount.x, 1.0f/currentBrick.vVoxelCount.y, 1.0f/currentBrick.vVoxelCount.z);

  float fSampleRateModifier = m_fSampleRateModifier / ((m_bDecreaseSamplingRateNow) ? m_fSampleDecFactor : 1.0f);
  float fStepScale =  sqrt(2.0)/fSampleRateModifier * (FLOATVECTOR3(m_pDataset->GetDomainSize())/FLOATVECTOR3(m_pDataset->GetDomainSize(m_iCurrentLOD))).maxVal();


  switch (m_eRenderMode) {
    case RM_1DTRANS: {
      GLSLProgram *shader = m_pProgram1DTrans[m_bUseLighting ? 1 : 0];
      shader->SetUniformVector("fStepScale", fStepScale);
      if (m_bUseLighting) {
        m_pProgram1DTrans[1]->SetUniformVector("vVoxelStepsize",
                                               vStep.x, vStep.y, vStep.z);
      }
      break;
    }
    case RM_2DTRANS: {
      GLSLProgram *shader = m_pProgram2DTrans[m_bUseLighting ? 1 : 0];
      shader->SetUniformVector("fStepScale", fStepScale);
      shader->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
      break;
    }
    case RM_ISOSURFACE: {
      GLSLProgram *shader;
      if (m_bAvoidSeperateCompositing) {
        shader = (m_pDataset->GetComponentCount() == 1) ?
                 m_pProgramIsoNoCompose : m_pProgramColorNoCompose;
      } else {
        shader = (m_pDataset->GetComponentCount() == 1) ?
                 m_pProgramIso : m_pProgramColor;
      }
      shader->SetUniformVector("vVoxelStepsize", vStep.x, vStep.y, vStep.z);
      break;
    }
    case RM_INVALID: T_ERROR("Invalid rendermode set"); break;
  }
}

void GLSBVR2D::EnableClipPlane(RenderRegion *renderRegion) {
  if(!m_bClipPlaneOn) {
    AbstrRenderer::EnableClipPlane(renderRegion);
    m_SBVRGeogen.EnableClipPlane();
    PLANE<float> plane(m_ClipPlane.Plane());
    m_SBVRGeogen.SetClipPlane(plane);
  }
}

void GLSBVR2D::DisableClipPlane(RenderRegion *renderRegion) {
  if(m_bClipPlaneOn) {
    AbstrRenderer::DisableClipPlane(renderRegion);
    m_SBVRGeogen.DisableClipPlane();
  }
}

void GLSBVR2D::Render3DPreLoop() {

  m_SBVRGeogen.SetSamplingModifier(m_fSampleRateModifier / ((m_bDecreaseSamplingRateNow) ? m_fSampleDecFactor : 1.0f));

  if(m_bClipPlaneOn) {
    m_SBVRGeogen.EnableClipPlane();
    PLANE<float> plane(m_ClipPlane.Plane());
    m_SBVRGeogen.SetClipPlane(plane);
  } else {
    m_SBVRGeogen.DisableClipPlane();
  }

  switch (m_eRenderMode) {
    case RM_1DTRANS    :  m_p1DTransTex->Bind(1);
                          m_pProgram1DTrans[m_bUseLighting ? 1 : 0]->Enable();
                          glEnable(GL_BLEND);
                          glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                          break;
    case RM_2DTRANS    :  m_p2DTransTex->Bind(1);
                          m_pProgram2DTrans[m_bUseLighting ? 1 : 0]->Enable();
                          glEnable(GL_BLEND);
                          glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                          break;
    case RM_ISOSURFACE :  if (m_bAvoidSeperateCompositing) {
                            if (m_pDataset->GetComponentCount() == 1)
                              m_pProgramIsoNoCompose->Enable();
                            else
                              m_pProgramColorNoCompose->Enable();
                            glEnable(GL_BLEND);
                            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                          } else {
                            glEnable(GL_DEPTH_TEST);
                          }
                          break;
    default    :  T_ERROR("Invalid rendermode set");
                          break;
  }

  m_SBVRGeogen.SetLODData( UINTVECTOR3(m_pDataset->GetDomainSize(m_iCurrentLOD))  );
}

void GLSBVR2D::RenderProxyGeometry() {

  for (UINT32 i = 0;i<3;i++) {

    switch (m_SBVRGeogen.m_vSliceTrianglesOrder[i]) {
      case SBVRGeogen2D::DIRECTION_X : {
                                          glBegin(GL_TRIANGLES);
                                            for (int i = int(m_SBVRGeogen.m_vSliceTrianglesX.size())-1;i>=0;i--) {
                                              glTexCoord3f(m_SBVRGeogen.m_vSliceTrianglesX[i].m_vTex.x,
                                                           m_SBVRGeogen.m_vSliceTrianglesX[i].m_vTex.y,
                                                           m_SBVRGeogen.m_vSliceTrianglesX[i].m_vTex.z);
                                              glVertex3f(m_SBVRGeogen.m_vSliceTrianglesX[i].m_vPos.x,
                                                         m_SBVRGeogen.m_vSliceTrianglesX[i].m_vPos.y,
                                                         m_SBVRGeogen.m_vSliceTrianglesX[i].m_vPos.z);
                                            }
                                          glEnd();
                                       } break;
      case SBVRGeogen2D::DIRECTION_Y : {
                                          glBegin(GL_TRIANGLES);
                                            for (int i = int(m_SBVRGeogen.m_vSliceTrianglesY.size())-1;i>=0;i--) {
                                              glTexCoord3f(m_SBVRGeogen.m_vSliceTrianglesY[i].m_vTex.x,
                                                           m_SBVRGeogen.m_vSliceTrianglesY[i].m_vTex.y,
                                                           m_SBVRGeogen.m_vSliceTrianglesY[i].m_vTex.z);
                                              glVertex3f(m_SBVRGeogen.m_vSliceTrianglesY[i].m_vPos.x,
                                                         m_SBVRGeogen.m_vSliceTrianglesY[i].m_vPos.y,
                                                         m_SBVRGeogen.m_vSliceTrianglesY[i].m_vPos.z);
                                            }
                                          glEnd();
                                       } break;
      case SBVRGeogen2D::DIRECTION_Z : {
                                          glBegin(GL_TRIANGLES);
                                            for (int i = int(m_SBVRGeogen.m_vSliceTrianglesZ.size())-1;i>=0;i--) {
                                              glTexCoord3f(m_SBVRGeogen.m_vSliceTrianglesZ[i].m_vTex.x,
                                                           m_SBVRGeogen.m_vSliceTrianglesZ[i].m_vTex.y,
                                                           m_SBVRGeogen.m_vSliceTrianglesZ[i].m_vTex.z);
                                              glVertex3f(m_SBVRGeogen.m_vSliceTrianglesZ[i].m_vPos.x,
                                                         m_SBVRGeogen.m_vSliceTrianglesZ[i].m_vPos.y,
                                                         m_SBVRGeogen.m_vSliceTrianglesZ[i].m_vPos.z);
                                            }
                                          glEnd();
                                       } break;
    }
  }



}

void GLSBVR2D::Render3DInLoop(size_t iCurrentBrick, int iStereoID) {
  const Brick& b = (iStereoID == 0) ? m_vCurrentBrickList[iCurrentBrick] : m_vLeftEyeBrickList[iCurrentBrick];

  // setup the slice generator
  m_SBVRGeogen.SetBrickData(b.vExtension, b.vVoxelCount,
                            b.vTexcoordsMin, b.vTexcoordsMax);
  FLOATMATRIX4 maBricktTrans;
  maBricktTrans.Translation(b.vCenter.x, b.vCenter.y, b.vCenter.z);
  FLOATMATRIX4 maBricktModelView = maBricktTrans * m_matModelView[iStereoID];
  m_mProjection[iStereoID].setProjection();
  maBricktModelView.setModelview();

  m_SBVRGeogen.SetWorld(maBricktTrans * m_mRotation * m_mTranslation);
  m_SBVRGeogen.SetView(m_mView[iStereoID], true);

  if (! m_bAvoidSeperateCompositing && m_eRenderMode == RM_ISOSURFACE) {
    glDisable(GL_BLEND);
    GLSLProgram* shader = (m_pDataset->GetComponentCount() == 1) ? m_pProgramIso : m_pProgramColor;

    m_TargetBinder.Bind(m_pFBOIsoHit[iStereoID], 0, m_pFBOIsoHit[iStereoID], 1);

    if (m_iBricksRenderedInThisSubFrame == 0) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->Enable();
    SetBrickDepShaderVars(b);
    shader->SetUniformVector("fIsoval", static_cast<float>
                                        (this->GetNormalizedIsovalue()));
    RenderProxyGeometry();
    shader->Disable();

    if (m_bDoClearView) {
      m_TargetBinder.Bind(m_pFBOCVHit[iStereoID], 0, m_pFBOCVHit[iStereoID], 1);

      if (m_iBricksRenderedInThisSubFrame == 0) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      m_pProgramIso->Enable();
      m_pProgramIso->SetUniformVector("fIsoval", static_cast<float>
                                                 (GetNormalizedCVIsovalue()));
      RenderProxyGeometry();
      m_pProgramIso->Disable();
    }
  } else {
    m_TargetBinder.Bind(m_pFBO3DImageCurrent[iStereoID]);

    glDepthMask(GL_FALSE);
    SetBrickDepShaderVars(b);
    RenderProxyGeometry();
    glDepthMask(GL_TRUE);
  }
  m_TargetBinder.Unbind();
}


void GLSBVR2D::Render3DPostLoop() {
  GLRenderer::Render3DPostLoop();

  // disable the shader
  switch (m_eRenderMode) {
    case RM_1DTRANS    :  m_pProgram1DTrans[m_bUseLighting ? 1 : 0]->Disable();
                          glDisable(GL_BLEND);
                          break;
    case RM_2DTRANS    :  m_pProgram2DTrans[m_bUseLighting ? 1 : 0]->Disable();
                          glDisable(GL_BLEND);
                          break;
    case RM_ISOSURFACE :  if (m_bAvoidSeperateCompositing) {
                            if (m_pDataset->GetComponentCount() == 1)
                              m_pProgramIsoNoCompose->Disable();
                            else
                              m_pProgramColorNoCompose->Disable();
                             glDisable(GL_BLEND);
                          }
                          break;
    case RM_INVALID    :  T_ERROR("Invalid rendermode set"); break;
  }
}

void GLSBVR2D::RenderHQMIPPreLoop(const RenderRegion2D &region) {
  GLRenderer::RenderHQMIPPreLoop(region);
  m_pProgramHQMIPRot->Enable();

  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_MAX);
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
}

void GLSBVR2D::RenderHQMIPInLoop(const Brick& b) {
  m_SBVRGeogen.SetBrickData(b.vExtension, b.vVoxelCount, b.vTexcoordsMin, b.vTexcoordsMax);
  FLOATMATRIX4 maBricktTrans;
  maBricktTrans.Translation(b.vCenter.x, b.vCenter.y, b.vCenter.z);
  if (m_bOrthoView)
    m_SBVRGeogen.SetView(FLOATMATRIX4());
  else
    m_SBVRGeogen.SetView(m_mView[0]);

  m_SBVRGeogen.SetWorld(maBricktTrans * m_maMIPRotation, true);

  RenderProxyGeometry();
}

void GLSBVR2D::RenderHQMIPPostLoop() {
  GLRenderer::RenderHQMIPPostLoop();
  m_pProgramHQMIPRot->Disable();
}


bool GLSBVR2D::LoadDataset(const string& strFilename, bool& bRebrickingRequired) {
  if (GLRenderer::LoadDataset(strFilename, bRebrickingRequired)) {
    UINTVECTOR3    vSize = UINTVECTOR3(m_pDataset->GetDomainSize());
    FLOATVECTOR3 vAspect = FLOATVECTOR3(m_pDataset->GetScale());
    vAspect /= vAspect.maxVal();

    m_SBVRGeogen.SetVolumeData(vAspect, vSize);
    return true;
  } else return false;
}

void GLSBVR2D::ComposeSurfaceImage(int iStereoID) {
  if (!m_bAvoidSeperateCompositing) GLRenderer::ComposeSurfaceImage(iStereoID);
}


void GLSBVR2D::UpdateColorsInShaders() {
  GLRenderer::UpdateColorsInShaders();

  FLOATVECTOR3 a = m_cAmbient.xyz()*m_cAmbient.w;
  FLOATVECTOR3 d = m_cDiffuse.xyz()*m_cDiffuse.w;
  FLOATVECTOR3 s = m_cSpecular.xyz()*m_cSpecular.w;
  FLOATVECTOR3 dir(0.0f,0.0f,-1.0f);  // so far the light source is always a headlight

  FLOATVECTOR3 scale = 1.0f/FLOATVECTOR3(m_pDataset->GetScale());

  m_pProgramIsoNoCompose->Enable();
  m_pProgramIsoNoCompose->SetUniformVector("vLightAmbient",a.x,a.y,a.z);
  m_pProgramIsoNoCompose->SetUniformVector("vLightDiffuse",d.x,d.y,d.z);
  m_pProgramIsoNoCompose->SetUniformVector("vLightSpecular",s.x,s.y,s.z);
  m_pProgramIsoNoCompose->SetUniformVector("vLightDir",dir.x,dir.y,dir.z);
  m_pProgramIsoNoCompose->SetUniformVector("vDomainScale",scale.x,scale.y,scale.z);
  m_pProgramIsoNoCompose->Disable();

  m_pProgramColorNoCompose->Enable();
  m_pProgramColorNoCompose->SetUniformVector("vLightAmbient",a.x,a.y,a.z);
  //m_pProgramColorNoCompose->SetUniformVector("vLightDiffuse",d.x,d.y,d.z); // only abient color is used in color-volume mode yet
  //m_pProgramColorNoCompose->SetUniformVector("vLightSpecular",s.x,s.y,s.z); // only abient color is used in color-volume mode yet
  m_pProgramColorNoCompose->SetUniformVector("vLightDir",dir.x,dir.y,dir.z);
  m_pProgramColorNoCompose->SetUniformVector("vDomainScale",scale.x,scale.y,scale.z);
  m_pProgramColorNoCompose->Disable();
}

/*
#include "GLTexture3D.h"

bool GLSBVR2D::BindVolumeTex(const BrickKey& bkey, const UINT64 iIntraFrameCounter) {
  m_p3DVolTex = NULL;
  return true;

  //m_pMasterController->MemMan()->Get3DTexture(m_pDataset, bkey, m_bUseOnlyPowerOfTwo, m_bDownSampleTo8Bits, m_bDisableBorder, iIntraFrameCounter, m_iFrameCounter);
  //if(m_p3DVolTex) {
  //  m_p3DVolTex->Bind(0);
  //  return true;
  //} else {
  //  return false;
  //}
}

bool GLSBVR2D::UnbindVolumeTex() {
  m_p3DVolTex = NULL;
  return true;

  //if(m_p3DVolTex) {
  //  m_pMasterController->MemMan()->Release3DTexture(m_p3DVolTex);
  //  m_p3DVolTex = NULL;
  //  return true;
  //} else {
  //  return false;
  //}
}
*/