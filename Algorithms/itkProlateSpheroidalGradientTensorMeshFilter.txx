/*=========================================================================

Program:   ImagingSciences
Module:    $Id: itkProlateSpheroidalGradientTensorMeshFilter.txx 1 2010-05-21 14:00:33Z nt08 $
Language:  C++
Author:    $Author: nt08 $
Date:      $Date: 2010-05-21 14:00:33 +0000 (Fri, 21 May 2010) $
Version:   $Revision: 1 $

Copyright (c) 2010 King's College London - Division of Imaging Sciences. All rights reserved.
See Copyright.txt for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itk_ProlateSpheroidalGradientTensorMeshFilter_txx_
#define _itk_ProlateSpheroidalGradientTensorMeshFilter_txx_

#include "itkProlateSpheroidalGradientTensorMeshFilter.h"
#include <cstdio>

namespace itk
{

  template<class TMesh>
  void
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::SetInput(unsigned int index, const TMesh* mesh )
  {
    
    // Process object is not const-correct so the const_cast is required here
    this->ProcessObject::SetNthInput(index, 
                                   const_cast< TMesh *>( mesh ) );
    
  }
  
  template<class TMesh>
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::ProlateSpheroidalGradientTensorMeshFilter()
  {
    m_UsePiWorkAround = 1;
    m_LogInput  = MeshType::New();
    m_LogOutput1 = MeshType::New();
    m_LogOutput2 = MeshType::New();
    m_LogOutput3 = MeshType::New();

    m_Transform = 0;
    
    this->SetNumberOfRequiredOutputs(3);
    this->SetNumberOfRequiredInputs(2);
    for(unsigned int i=1; i<3; i++)
    {
      this->SetNthOutput (i, MeshType::New());
    }

    this->ReleaseDataBeforeUpdateFlagOff();
  };
  
  template<class TMesh>
  void
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::EvaluateAtIndex(unsigned long index)
  {

    MeshType* input   = m_LogInput;
    MeshType* output1 = m_LogOutput1;
    MeshType* output2 = m_LogOutput2;
    MeshType* output3 = m_LogOutput3;

    InternalMatrixType USigma (input->GetNumberOfPoints(), 3);
    InternalMatrixType dUl    (input->GetNumberOfPoints(), TensorType::DegreesOfFreedom);
    this->EvaluateUSigmaAnddUl (index, USigma, dUl);

    SolverType solver (USigma);
    // solve USigma . gradl = dUl
    InternalMatrixType gradl = solver.solve (dUl);

    output1->SetPointData (index, this->vec2tensor (gradl.get_row (0)));
    output2->SetPointData (index, this->vec2tensor (gradl.get_row (1)));
    output3->SetPointData (index, this->vec2tensor (gradl.get_row (2)));
  }

  template<class TMesh>
  void
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>::EvaluateUSigmaAnddUl (unsigned long index, InternalMatrixType USigma, InternalMatrixType dUl)
  {
    typedef typename MeshType::PointDataContainer  PixelContainer;
    typedef typename MeshType::PointsContainer     PointContainer;
    typename PointContainer::Pointer points  = m_LogInput->GetPoints();
    typename PixelContainer::Pointer tensors = m_LogInput->GetPointData();
    typename PointContainer::Iterator p_it = points->Begin();
    typename PixelContainer::Iterator t_it = tensors->Begin();

    MeshType* output1 = m_LogOutput1;
    typename PointType::ValueType zeros[3] = {0,0,0};
    
    PointType p  (zeros);
    TensorType t (static_cast<ScalarType>(0.0));
    PointType ptn;
    TensorType tn;
    VectorType u_i;
    TensorType duil;
    InternalMatrixType Sigma;
    InternalVectorType u_i_Sigma;
    
    output1->GetPoint (index, & p);
    output1->GetPointData (index, & t);
    
    unsigned long counter = 0;
    
    while( p_it != points->End())
    {
      ptn = p_it.Value();
      tn = t_it.Value();
      
      u_i = ptn - p;
      duil = tn - t;
      
      Sigma = this->EvaluateSigma (ptn);

      if (m_UsePiWorkAround)
      {
	while (u_i[2] >= vnl_math::pi)
	  u_i[2] -= 2 * vnl_math::pi;
	while (u_i[2] < - vnl_math::pi)
	  u_i[2] += 2 * vnl_math::pi;
	while (u_i[1] >= vnl_math::pi)
	  u_i[1] -= 2 * vnl_math::pi;
	while (u_i[1] < - vnl_math::pi)
	  u_i[1] += 2 * vnl_math::pi;
      }      

      u_i_Sigma = Sigma * u_i.GetVnlVector();
      
      USigma.set_row (counter, u_i_Sigma.data_block());
      dUl.set_row    (counter, this->tensor2vec (duil));
      
      ++p_it;
      ++t_it;
      counter++;
    }
    
  }
  
  template<class TMesh>
  typename ProlateSpheroidalGradientTensorMeshFilter<TMesh>::InternalMatrixType
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>::EvaluateSigma (PointType p)
  {
    // we need to grab the prolate spheroidal transform
    // and get the scaling factors
    double h[3] = {1,1,1};
    InternalMatrixType m (3,3);

    if (m_UsePiWorkAround)
    {
      if (m_Transform.IsNull())
      {
	itkWarningMacro (<<"Prolate Spheroidal Transform is null, cannot estimate Sigma\n");
      }
      else
	m_Transform->EvaluateScaleFactors (p.GetDataPointer(), h);
    }
    
    m[0][0] = h[0];
    m[1][1] = h[1];
    m[2][2] = h[2];

    return m;
  }
  
  
  
  template<class TMesh>
  void
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::BeforeThreadedGenerateData()
  {
    std::cout<<"tensors LOG"<<std::endl;
    
    typedef typename MeshType::PointDataContainer  PixelContainer;
    typename PixelContainer::ConstPointer pixels = this->GetInput(1)->GetPointData();
    typename PixelContainer::ConstPointer samplingpixels = this->GetInput(0)->GetPointData();
    
    typename PixelContainer::Pointer logpixels  = m_LogInput->GetPointData();
    typename PixelContainer::Pointer logpixels1 = m_LogOutput1->GetPointData();
    typename PixelContainer::Pointer logpixels2 = m_LogOutput2->GetPointData();
    typename PixelContainer::Pointer logpixels3 = m_LogOutput3->GetPointData();
    typename PixelContainer::ConstIterator it = pixels->Begin();
    typename PixelContainer::Iterator  log_it = logpixels->Begin();
    
    while( it != pixels->End() )
    {
      if (!it.Value().IsFinite() || it.Value().HasNans())
	std::cout<<"T is given not finite at "<<it.Value()<<std::endl;
      else
	log_it.Value() = it.Value().Log();
      
      ++it;
      ++log_it;
    }

    it  = samplingpixels->Begin();
    typename PixelContainer::Iterator      out_it1  = logpixels1->Begin();
    typename PixelContainer::Iterator      out_it2  = logpixels2->Begin();
    typename PixelContainer::Iterator      out_it3  = logpixels3->Begin();
    TensorType t;
    while(it != samplingpixels->End())
    {
      t = it.Value().Log();
      out_it1.Value() = t;
      out_it2.Value() = t;
      out_it3.Value() = t;
      ++it; ++out_it1; ++out_it2; ++out_it3;
    }
    
  }
  
  template<class TMesh>
  void ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::AfterThreadedGenerateData()
  {

    std::cout<<"tensors EXP"<<std::endl;

    typedef typename MeshType::PointDataContainer  PixelContainer;
    typename PixelContainer::Pointer pixels1    = this->GetOutput(0)->GetPointData();
    typename PixelContainer::Pointer pixels2    = this->GetOutput(1)->GetPointData();
    typename PixelContainer::Pointer pixels3    = this->GetOutput(2)->GetPointData();
    typename PixelContainer::Pointer logpixels1 =       m_LogOutput1->GetPointData();
    typename PixelContainer::Pointer logpixels2 =       m_LogOutput2->GetPointData();
    typename PixelContainer::Pointer logpixels3 =       m_LogOutput3->GetPointData();
    typename PixelContainer::Iterator it1    =    pixels1->Begin();
    typename PixelContainer::Iterator it2    =    pixels2->Begin();
    typename PixelContainer::Iterator it3    =    pixels3->Begin();
    typename PixelContainer::Iterator logit1 = logpixels1->Begin();
    typename PixelContainer::Iterator logit2 = logpixels2->Begin();
    typename PixelContainer::Iterator logit3 = logpixels3->Begin();

    while( logit1 != logpixels1->End() )
    {
      if (!logit1.Value().IsFinite() || logit1.Value().HasNans())
	std::cout<<"T1 is given not finite at "<<logit1.Value()<<std::endl;
      else
	it1.Value() = logit1.Value().Exp();
      if (!logit2.Value().IsFinite() || logit2.Value().HasNans())
	std::cout<<"T2 is given not finite at "<<logit2.Value()<<std::endl;
      else
	it2.Value() = logit2.Value().Exp();
      if (!logit3.Value().IsFinite() || logit3.Value().HasNans())
	std::cout<<"T3 is given not finite at "<<logit3.Value()<<std::endl;
      else
	it3.Value() = logit3.Value().Exp();
      
      ++it1; ++logit1;
      ++it2; ++logit2;
      ++it3; ++logit3;
      
    }
    
  }

  
  template<class TMesh>
  void 
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::GenerateData()
  {
    std::cout<<"Gradient Evaluation starting"<<std::endl;
    
    // Call a method that can be overridden by a subclass to perform
    // some calculations prior to splitting the main computations into
    // separate threads
    this->BeforeThreadedGenerateData();
    
    // Set up the multithreaded processing
    ThreadStruct str;
    str.Filter = this;
    
    this->GetMultiThreader()->SetNumberOfThreads(this->GetNumberOfThreads());
    this->GetMultiThreader()->SetSingleMethod(this->ThreaderCallback, &str);

    std::cout<<"gradient-evaluation..."<<std::endl;
    std::cout<<"|";
    
    // multithread the execution
    this->GetMultiThreader()->SingleMethodExecute();

    std::cout<<"|"<<std::endl<<"done."<<std::endl;

    // Call a method that can be overridden by a subclass to perform
    // some calculations after all the threads have completed
    this->AfterThreadedGenerateData();

  }
  
  template<class TMesh>
  void
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::GenerateOutputInformation()
  {
    typedef typename MeshType::PointsContainer    PointContainer;
    typedef typename MeshType::PointDataContainer PixelContainer;
    
    // call the superclass's implementation of this method
    Superclass::GenerateOutputInformation();
    
    typename MeshType::Pointer      output1  = this->GetOutput(0);
    typename MeshType::Pointer      output2  = this->GetOutput(1);
    typename MeshType::Pointer      output3  = this->GetOutput(2);
    typename MeshType::ConstPointer input1  = this->GetInput(0);
    typename MeshType::ConstPointer input2  = this->GetInput(1);

    if( !input1)
    {
      itkExceptionMacro(<<"Missing Input Mesh (sampling points)");
    }
    
    if( !input2)
    {
      itkExceptionMacro(<<"Missing Input Mesh (data)");
    }
    
    if( !output1 || !output2 || !output3)
    {
      itkExceptionMacro(<<"Missing Output Meshes");
    }

    this->CopyInputMeshToOutputMeshPoints();
    
    typename PixelContainer::Pointer outputdata   = PixelContainer::New();    
    outputdata->Reserve (input1->GetNumberOfPoints());
    output1->SetPointData (outputdata);
    output2->SetPointData (outputdata);
    output3->SetPointData (outputdata);

    typename PointContainer::ConstPointer datapoints = input2->GetPoints();
    typename PointContainer::ConstPointer samplingpoints = input1->GetPoints();
    typename PixelContainer::ConstPointer datapixels = input2->GetPointData();
    typename PixelContainer::ConstPointer samplingpixels = input1->GetPointData();
    
    typename PointContainer::Pointer logdatapoints = PointContainer::New();
    typename PointContainer::Pointer logsamplingpoints1 = PointContainer::New();
    typename PointContainer::Pointer logsamplingpoints2 = PointContainer::New();
    typename PointContainer::Pointer logsamplingpoints3 = PointContainer::New();
    typename PixelContainer::Pointer logdatapixels = PixelContainer::New();
    typename PixelContainer::Pointer logsamplingpixels1 = PixelContainer::New();
    typename PixelContainer::Pointer logsamplingpixels2 = PixelContainer::New();
    typename PixelContainer::Pointer logsamplingpixels3 = PixelContainer::New();
    logdatapoints->Reserve (datapoints->Size());
    logsamplingpoints1->Reserve (samplingpoints->Size());
    logsamplingpoints2->Reserve (samplingpoints->Size());
    logsamplingpoints3->Reserve (samplingpoints->Size());
    logdatapixels->Reserve (datapixels->Size());
    logsamplingpixels1->Reserve (samplingpoints->Size());
    logsamplingpixels2->Reserve (samplingpoints->Size());
    logsamplingpixels3->Reserve (samplingpoints->Size());
    
    m_LogInput->SetPointData  (logdatapixels);
    m_LogOutput1->SetPointData (logsamplingpixels1);
    m_LogOutput2->SetPointData (logsamplingpixels2);
    m_LogOutput3->SetPointData (logsamplingpixels3);
    m_LogInput->SetPoints  (logdatapoints);
    m_LogOutput1->SetPoints (logsamplingpoints1);
    m_LogOutput2->SetPoints (logsamplingpoints2);
    m_LogOutput3->SetPoints (logsamplingpoints3);

    typename PointContainer::ConstIterator  in_it  = datapoints->Begin();
    typename PointContainer::Iterator      out_it  = logdatapoints->Begin();
    while(in_it != datapoints->End())
    {
      out_it.Value() = in_it.Value();
      ++in_it; ++out_it;
    }
    
    in_it  = samplingpoints->Begin();
    typename PointContainer::Iterator      out_it1  = logsamplingpoints1->Begin();
    typename PointContainer::Iterator      out_it2  = logsamplingpoints2->Begin();
    typename PointContainer::Iterator      out_it3  = logsamplingpoints3->Begin();
    PointType  p;
    while(in_it != samplingpoints->End())
    {
      p = in_it.Value();
      out_it1.Value() = p;
      out_it2.Value() = p;
      out_it3.Value() = p;
      ++in_it; ++out_it1; ++out_it2; ++out_it3;
    }
    
  }
  
  template<class TMesh>
  void
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::ThreadedGenerateData(unsigned long range[2], int id)
  {
    unsigned long start = range[0];
    unsigned long end   = start + range[1];

    double percent = 0;
    int done = 0;
    double threads = this->GetNumberOfThreads();
    
    for (unsigned long i = start; i < end; i++)
    {

      percent += 100 * 1.0 / (double)( threads * (end - start) );
      if (std::floor (percent) >= done)
      {
	done++;
	std::cout<<"="<<std::flush;
      }
      try
      {
	this->EvaluateAtIndex (i);
      }
      catch (itk::ExceptionObject & e)
      {
	std::cerr << e << std::endl;
	continue;
      }
    }
  }

  template<class TMesh>
  int
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::SplitRequestedRegion(int i, int num, unsigned long range[2])
  {
    MeshType* output  = this->GetOutput();

    if( !output )
    {
      itkExceptionMacro(<<"Missing Output Mesh");
    }
    
    // determine the actual number of pieces that will be generated
    unsigned long totalnumber;
    totalnumber = output->GetNumberOfPoints();
    
    int valuesPerThread = (int)::vcl_ceil(totalnumber/(double)num);
    int maxThreadIdUsed = (int)::vcl_ceil(totalnumber/(double)valuesPerThread) - 1;

    range[0] = 0;
    range[1] = totalnumber;
    
    if (i < maxThreadIdUsed)
    {
      range[0] += i*valuesPerThread;
      range[1] = valuesPerThread;
    }
    if (i == maxThreadIdUsed)
    {
      range[0] += i*valuesPerThread;
      range[1] = range[1] - i*valuesPerThread;
    }
  
    return maxThreadIdUsed + 1;
  }  


  // Callback routine used by the threading library. This routine just calls
  // the ThreadedGenerateData method after setting the correct region for this
  // thread. 
  
  template<class TMesh>
  ITK_THREAD_RETURN_TYPE
  ProlateSpheroidalGradientTensorMeshFilter<TMesh>
  ::ThreaderCallback( void *arg )
  {
    ThreadStruct *str;
    int total, threadId, threadCount;
    
    threadId = ((MultiThreader::ThreadInfoStruct *)(arg))->ThreadID;
    threadCount = ((MultiThreader::ThreadInfoStruct *)(arg))->NumberOfThreads;
    
    str = (ThreadStruct *)(((MultiThreader::ThreadInfoStruct *)(arg))->UserData);
    
    // execute the actual method with appropriate output region
    // first find out how many pieces extent can be split into.
    unsigned long range[2];
    
    total = str->Filter->SplitRequestedRegion(threadId, threadCount, range);
    
    if (threadId < total)
    {
      str->Filter->ThreadedGenerateData(range, threadId);
    }
    // else
    //   {
    //   otherwise don't use this thread. Sometimes the threads dont
    //   break up very well and it is just as efficient to leave a 
    //   few threads idle.
    //   }
    
    return ITK_THREAD_RETURN_VALUE;
  }
  
  
} // end of namespace itk

#endif