/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    GeodesicActiveContourImageFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

// Software Guide : BeginLatex
//
// The following example illustrates the use of the
// \doxygen{GeodesicActiveContourLevelSetImageFilter}.  The
// implementation of this filter in ITK is based on the paper by
// Caselles \cite{Caselles1997}.  This implementation extends the
// functionality of the \doxygen{ShapeDetectionLevelSetImageFilter} by the
// addition of a third avection term which attracts the level set to
// the object boundaries.
//
// \doxygen{GeodesicActiveContourLevelSetImageFilter} expects two
// inputs.  The first is an initial level set in the form of an
// \doxygen{Image}. The second input is a feature image. For this
// algorithm, the feature image is an edge potential image that
// basically follows the same rules used for the
// \doxygen{ShapeDetectionLevelSetImageFilter} discussed in
// section~\ref{sec:ShapeDetectionLevelSetFilter}.  The configuration
// of this example is quite similar to the example on the use of the
// \doxygen{ShapeDetectionLevelSetImageFilter}. We omit most of the
// redundant description. A look at the code will reveal the great
// degree of similarity between both examples.
//
// \begin{figure} \center
// \includegraphics[width=15cm]{GeodesicActiveContoursCollaborationDiagram1.eps}
// \caption[GeodesicActiveContourLevelSetImageFilter collaboration
// diagram]{Collaboration diagram for the GeodesicActiveContourLevelSetImageFilter
// applied to a segmentation task.}
// \label{fig:GeodesicActiveContoursCollaborationDiagram}
// \end{figure}
//
// Figure~\ref{fig:GeodesicActiveContoursCollaborationDiagram} shows the major
// components involved in the application of the
// \doxygen{GeodesicActiveContourLevelSetImageFilter} to a segmentation task.
// This pipeline is quite similar to the one used by the
// \doxygen{ShapeDetectionLevelSetImageFilter} in
// section~\ref{sec:ShapeDetectionLevelSetFilter}.
//
// The pipeline involves a first stage of smoothing using the
// \doxygen{CurvatureAnisotropicDiffusionImageFilter}. The smoothed image is
// passed as the input to the
// \doxygen{GradientMagnitudeRecursiveGaussianImageFilter} and then to the
// \doxygen{SigmoidImageFilter} in order to produce the edge potential image.
// A set of user-provided seeds is passed to a
// \doxygen{FastMarchingImageFilter} in order to compute the distance map. A
// constant value is subtracted from this map in order to obtain a level set in
// which the \emph{zero set} represents the initial contour. This level set is
// also passed as input to the \doxygen{GeodesicActiveContourLevelSetImageFilter}.
// 
// Finally, the level set at the output of the
// \doxygen{GeodesicActiveContourLevelSetImageFilter} is passed to a
// \doxygen{BinaryThresholdImageFilter} in order to produce a binary mask
// representing the segmented object.
//
// Let's start by including the headers of the main filters involved in the
// preprocessing. 
//
// Software Guide : EndLatex 


// Software Guide : BeginCodeSnippet
#include "itkImage.h"
#include "itkGeodesicActiveContourLevelSetImageFilter.h"
// Software Guide : EndCodeSnippet




#include "itkCurvatureAnisotropicDiffusionImageFilter.h"
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkSigmoidImageFilter.h"
#include "itkFastMarchingImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"


int main( int argc, char *argv[] )
{


  if( argc < 10 )
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " inputImage  outputImage";
    std::cerr << " seedX seedY InitialDistance";
    std::cerr << " Sigma SigmoidAlpha SigmoidBeta";
    std::cerr << " PropagationScaling"  << std::endl;
    return 1;
    }




  //  Software Guide : BeginLatex
  //  
  //  We now declare the image type using a pixel type and a particular
  //  dimension. In this case the \code{float} type is used for the pixels due
  //  to the requirements of the smoothing filter. 
  //
  //  Software Guide : EndLatex 

  // Software Guide : BeginCodeSnippet
  typedef   float           InternalPixelType;
  const     unsigned int    Dimension = 2;
  
  typedef itk::Image< InternalPixelType, Dimension >  InternalImageType;
  // Software Guide : EndCodeSnippet
                                     

  //  
  //  The following lines instantiate the thresholding filter that will
  //  process the final level set at the output of the GeodesicActiveContourLevelSetImageFilter.                                    
  //
  typedef unsigned char OutputPixelType;

  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;

  typedef itk::BinaryThresholdImageFilter< 
                        InternalImageType, 
                        OutputImageType    >    ThresholdingFilterType;
  
  ThresholdingFilterType::Pointer thresholder = ThresholdingFilterType::New();
                        
  thresholder->SetLowerThreshold( -1000.0 );
  thresholder->SetUpperThreshold(     0.0 );

  thresholder->SetOutsideValue(  0  );
  thresholder->SetInsideValue(  255 );


  //  
  // We instantiate reader and writer types in the following lines.
  //

  typedef  itk::ImageFileReader< InternalImageType > ReaderType;
  typedef  itk::ImageFileWriter<  OutputImageType  > WriterType;


  ReaderType::Pointer reader = ReaderType::New();
  WriterType::Pointer writer = WriterType::New();

  reader->SetFileName( argv[1] );
  writer->SetFileName( argv[2] );



  //
  //  The RescaleIntensityImageFilter type is declared below. This filter will
  //  renormalize image before sending them to writers.
  //
  typedef itk::RescaleIntensityImageFilter< 
                               InternalImageType, 
                               OutputImageType >   CastFilterType;

  //  
  //  The \doxygen{CurvatureAnisotropicDiffusionImageFilter} type is
  //  instantiated using the internal image type. 
  //
  typedef   itk::CurvatureAnisotropicDiffusionImageFilter< 
                               InternalImageType, 
                               InternalImageType >  SmoothingFilterType;

  SmoothingFilterType::Pointer smoothing = SmoothingFilterType::New();


  //  
  //  The types of the \doxygen{GradientMagnitudeRecursiveGaussianImageFilter} and
  //  \doxygen{SigmoidImageFilter} are instantiated using the internal
  //  image type. 
  //
  typedef   itk::GradientMagnitudeRecursiveGaussianImageFilter< 
                               InternalImageType, 
                               InternalImageType >  GradientFilterType;

  typedef   itk::SigmoidImageFilter<                               
                               InternalImageType, 
                               InternalImageType >  SigmoidFilterType;

  GradientFilterType::Pointer  gradientMagnitude = GradientFilterType::New();

  SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();


  //
  //  The minimum and maximum values of the \doxygen{SigmoidImageFilter} output
  //  are defined with the methods \code{SetOutputMinimum()} and
  //  \code{SetOutputMaximum()}. In our case, we want these two values to be
  //  $0.0$ and $1.0$ respectively in order to get a nice speed image to feed
  //  the \code{FastMarchingImageFilter}. Additional details on the user of the
  //  \doxygen{SigmoidImageFilter} are presented in
  //  section~\ref{sec:IntensityNonLinearMapping}.
  //

  sigmoid->SetOutputMinimum(  0.0  );
  sigmoid->SetOutputMaximum(  1.0  );


  //  
  //  We declare now the type of the \doxygen{FastMarchingImageFilter} that
  //  will be used to generate the initial level set in the form of a distance
  //  map.
  //
  typedef  itk::FastMarchingImageFilter< 
                              InternalImageType, 
                              InternalImageType >    FastMarchingFilterType;


  //  
  //  then, we  construct one filter of this class using the \code{New()} method. 
  //
  FastMarchingFilterType::Pointer  fastMarching = FastMarchingFilterType::New();

  
  //  Software Guide : BeginLatex
  //  
  //  In the following lines we instantiate the type of the
  //  \doxygen{GeodesicActiveContourLevelSetImageFilter} and create an object of this type
  //  using the \code{New()} method.
  //
  //  Software Guide : EndLatex 

  // Software Guide : BeginCodeSnippet
  typedef  itk::GeodesicActiveContourLevelSetImageFilter< 
                              InternalImageType, 
                              InternalImageType >    GeodesicActiveContourFilterType;

  GeodesicActiveContourFilterType::Pointer geodesicActiveContour = 
                                     GeodesicActiveContourFilterType::New();                              
  // Software Guide : EndCodeSnippet


  
  //  Software Guide : BeginLatex
  //  
  //  For the \doxygen{GeodesicActiveContourLevelSetImageFilter}, scaling
  //  parameters are used to trade off between the propagation (inflation),
  //  the curvature (smoothing) and the advection terms. These parameters are
  //  set using methods \code{SetPropagationScaling()}, \code{SetCurvatureScaling()}
  //  and \code{SetAdvectionScaling()}. In this example, we will set the
  //  curvature and advection scales to one and let the propagation scale
  //  be a command-line argument.
  //
  //  \index{itk::GeodesicActiveContourLevelSetImageFilter!SetPropagationScaling()}
  //  \index{itk::SegmentationLevelSetImageFilter!SetPropagationScaling()}
  //  \index{itk::GeodesicActiveContourLevelSetImageFilter!SetCurvatureScaling()}
  //  \index{itk::SegmentationLevelSetImageFilter!SetCurvatureScaling()}
  //  \index{itk::GeodesicActiveContourLevelSetImageFilter!SetAdvectionScaling()}
  //  \index{itk::SegmentationLevelSetImageFilter!SetAdvectionScaling()}
  //
  //  Software Guide : EndLatex 

  const double propagationScaling = atof( argv[9] );

  //  Software Guide : BeginCodeSnippet
  geodesicActiveContour->SetPropagationScaling( propagationScaling );
  geodesicActiveContour->SetCurvatureScaling( 1.0 );
  geodesicActiveContour->SetAdvectionScaling( 1.0 );
  //  Software Guide : EndCodeSnippet 

  //  Once activiated the level set evolution stop if the convergence criteria has
  //  been reached or if the maximum number of iterations has elasped.
  //  The convergence criteria is defined in terms of the root mean squared (RMS)
  //  change in the level set function. The evolution is said to have converged
  //  if the RMS change is below a user specified threshold.
  //  In a real application is desirable to couple the evolution of the
  //  zero set to a visualization module allowing the user to follow the
  //  evolution of the zero set. With this feedback, the user may decide when
  //  to stop the algorithm before the zero set leaks through the regions of
  //  low gradient in the contour of the anatomical structure to be segmented.

  geodesicActiveContour->SetMaximumRMSError( 0.02 );
  geodesicActiveContour->SetMaximumIterations( 800 );


  //  Software Guide : BeginLatex
  //  
  //  The filters are now connected in a pipeline indicated in
  //  Figure~\ref{fig:GeodesicActiveContoursCollaborationDiagram} using the following
  //  lines: 
  //
  //  Software Guide : EndLatex 

  // Software Guide : BeginCodeSnippet
  smoothing->SetInput( reader->GetOutput() );

  gradientMagnitude->SetInput( smoothing->GetOutput() );

  sigmoid->SetInput( gradientMagnitude->GetOutput() );

  geodesicActiveContour->SetInput(           fastMarching->GetOutput()     );
  geodesicActiveContour->SetFeatureImage(       sigmoid->GetOutput()          );

  thresholder->SetInput( geodesicActiveContour->GetOutput() );

  writer->SetInput( thresholder->GetOutput() );
  // Software Guide : EndCodeSnippet




  //
  //  The \doxygen{CurvatureAnisotropicDiffusionImageFilter} requires a couple
  //  of parameter to be defined. The following are typical values for $2D$
  //  images. However they may have to be adjusted depending on the amount of
  //  noise present in the input image. This filter has been discussed in
  //  section~\ref{sec:GradientAnisotropicDiffusionImageFilter}.
  //
  
  smoothing->SetTimeStep( 0.125 );
  smoothing->SetNumberOfIterations(  5 );
  smoothing->SetConductanceParameter( 3.0 );




  //
  //  The \doxygen{GradientMagnitudeRecursiveGaussianImageFilter} performs the
  //  equivalent of a convolution with a Gaussian kernel, followed by a
  //  derivative operator. The sigma of this Gaussian can be used to control
  //  the range of influence of the image edges. This filter has been discussed
  //  in section~\ref{sec:GradientMagnitudeRecursiveGaussianImageFilter}
  //

  const double sigma = atof( argv[6] );

  gradientMagnitude->SetSigma(  sigma  );



  //
  //  The \doxygen{SigmoidImageFilter} requires two parameters that define the
  //  linear transformation to be applied to the sigmoid argument. This
  //  parameters have been discussed in sections~\ref{sec:IntensityNonLinearMapping}
  //  and \ref{sec:FastMarchingImageFilter}.
  //

  const double alpha =  atof( argv[7] );
  const double beta  =  atof( argv[8] );


  sigmoid->SetAlpha( alpha );
  sigmoid->SetBeta(  beta  );



  
  //
  //  The \doxygen{FastMarchingImageFilter} requires the user to provide a seed
  //  point from which the level set will be generated. The user can actually
  //  pass not only one seed point but a set of them. Note the the
  //  \doxygen{FastMarchingImageFilter} is used here only as a helper in the
  //  determination of an initial Level Set. We could have used the
  //  \doxygen{DanielssonDistanceMapImageFilter} in the same way.
  //
  //
  //  The seeds are passed stored in a container. The type of this
  //  container is defined as \code{NodeContainer} among the
  //  \doxygen{FastMarchingImageFilter} traits.
  //

  typedef FastMarchingFilterType::NodeContainer           NodeContainer;
  typedef FastMarchingFilterType::NodeType                NodeType;

  NodeContainer::Pointer seeds = NodeContainer::New();
  

  InternalImageType::IndexType  seedPosition;
  
  seedPosition[0] = atoi( argv[3] );
  seedPosition[1] = atoi( argv[4] );


  //
  //  Nodes are created as stack variables and initialized with a value and an
  //  \doxygen{Index} position. Note that here we assign the value of minus the
  //  user-provided distance to the unique node of the seeds passed to the
  //  \doxygen{FastMarchingImageFilter}. In this way, the value will increment
  //  as the front is propagated, until it reaches the zero value corresponding
  //  to the contour. After this, the front will continue propagating until it
  //  fills up the entire image. The initial distance is taken here from the
  //  command line arguments. The rule of thumb for the user is to select this
  //  value as the distance from the seed points at which he want the initial
  //  contour to be.
  //

  const double initialDistance = atof( argv[5] );

  NodeType node;

  const double seedValue = - initialDistance;
  
  node.SetValue( seedValue );
  node.SetIndex( seedPosition );



  //
  //  The list of nodes is initialized and then every node is inserted using
  //  the \code{InsertElement()}.
  //

  seeds->Initialize();
  seeds->InsertElement( 0, node );




  //
  //  The set of seed nodes is passed now to the
  //  \doxygen{FastMarchingImageFilter} with the method
  //  \code{SetTrialPoints()}.
  //

  fastMarching->SetTrialPoints(  seeds  );




  //  
  //  Since the \doxygen{FastMarchingImageFilter} is used here just as a
  //  Distance Map generator. It does not require a speed image as input.
  //  Instead the constant value $1.0$ is passed using the
  //  \code{SetSpeedConstant()} method.
  //
  
  fastMarching->SetSpeedConstant( 1.0 );


  //
  //  Here we configure all the writers required to see the intermediate
  //  outputs of the pipeline. This is added here only for
  //  pedagogical/debugging purposes. These intermediate output are normaly not
  //  required. Only the output of the final thresholding filter should be
  //  relevant.  Observing intermediate output is helpful in the process of
  //  fine tunning the parameters of filters in the pipeline. 
  //
  //
  CastFilterType::Pointer caster1 = CastFilterType::New();
  CastFilterType::Pointer caster2 = CastFilterType::New();
  CastFilterType::Pointer caster3 = CastFilterType::New();
  CastFilterType::Pointer caster4 = CastFilterType::New();

  WriterType::Pointer writer1 = WriterType::New();
  WriterType::Pointer writer2 = WriterType::New();
  WriterType::Pointer writer3 = WriterType::New();
  WriterType::Pointer writer4 = WriterType::New();

  caster1->SetInput( smoothing->GetOutput() );
  writer1->SetInput( caster1->GetOutput() );
  writer1->SetFileName("GeodesicActiveContourImageFilterOutput1.png");
  caster1->SetOutputMinimum(   0 );
  caster1->SetOutputMaximum( 255 );
  writer1->Update();

  caster2->SetInput( gradientMagnitude->GetOutput() );
  writer2->SetInput( caster2->GetOutput() );
  writer2->SetFileName("GeodesicActiveContourImageFilterOutput2.png");
  caster2->SetOutputMinimum(   0 );
  caster2->SetOutputMaximum( 255 );
  writer2->Update();

  caster3->SetInput( sigmoid->GetOutput() );
  writer3->SetInput( caster3->GetOutput() );
  writer3->SetFileName("GeodesicActiveContourImageFilterOutput3.png");
  caster3->SetOutputMinimum(   0 );
  caster3->SetOutputMaximum( 255 );
  writer3->Update();

  caster4->SetInput( fastMarching->GetOutput() );
  writer4->SetInput( caster4->GetOutput() );
  writer4->SetFileName("GeodesicActiveContourImageFilterOutput4.png");
  caster4->SetOutputMinimum(   0 );
  caster4->SetOutputMaximum( 255 );
  



  //
  //  The \doxygen{FastMarchingImageFilter} requires the user to specify the
  //  size of the image to be produced as output. This is done using the
  //  \code{SetOutputSize()}. Note that the size is obtained here from the
  //  output image of the smoothing filter. The size of this image is valid
  //  only after the \code{Update()} methods of this filter has been called
  //  directly or indirectly.
  //

  fastMarching->SetOutputSize( 
           reader->GetOutput()->GetBufferedRegion().GetSize() );

  
  //  Software Guide : BeginLatex
  //  
  //  The invocation of the \code{Update()} method on the writer triggers the
  //  execution of the pipeline.  As usual, the call is placed in a
  //  \code{try/catch} block should any errors occur or exceptions be thrown.
  //
  //  Software Guide : EndLatex 

  // Software Guide : BeginCodeSnippet
  try
    {
    writer->Update();
    }
  catch( itk::ExceptionObject & excep )
    {
    std::cerr << "Exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    }
  // Software Guide : EndCodeSnippet

  // Print out some useful information 
  std::cout << std::endl;
  std::cout << "Max. no. iterations: " << geodesicActiveContour->GetMaximumIterations() << std::endl;
  std::cout << "Max. RMS error: " << geodesicActiveContour->GetMaximumRMSError() << std::endl;
  std::cout << std::endl;
  std::cout << "No. elpased iterations: " << geodesicActiveContour->GetElapsedIterations() << std::endl;
  std::cout << "RMS change: " << geodesicActiveContour->GetRMSChange() << std::endl;

  writer4->Update();


  //
  // The following writer type is used to save the output of the time-crossing
  // map in a file with apropiate pixel representation. The advantage of saving
  // this image in native format is that it can be used with a viewer to help
  // determine an appropriate threshold to be used on the output of the
  // fastmarching filter.
  //
  typedef itk::ImageFileWriter< InternalImageType > InternalWriterType;

  InternalWriterType::Pointer mapWriter = InternalWriterType::New();
  mapWriter->SetInput( fastMarching->GetOutput() );
  mapWriter->SetFileName("GeodesicActiveContourImageFilterOutput4.mha");
  mapWriter->Update();

  InternalWriterType::Pointer speedWriter = InternalWriterType::New();
  speedWriter->SetInput( sigmoid->GetOutput() );
  speedWriter->SetFileName("GeodesicActiveContourImageFilterOutput3.mha");
  speedWriter->Update();


  InternalWriterType::Pointer gradientWriter = InternalWriterType::New();
  gradientWriter->SetInput( gradientMagnitude->GetOutput() );
  gradientWriter->SetFileName("GeodesicActiveContourImageFilterOutput2.mha");
  gradientWriter->Update();



  //  Software Guide : BeginLatex
  //
  //  Let's now run this example using as input the image
  //  \code{BrainProtonDensitySlice.png} provided in the directory
  //  \code{Insight/Examples/Data}. We can easily segment the major anatomical
  //  structures by providing seeds in the appropriate locations. The following
  //  table presents the parameters used for some structures.
  //
  //  \begin{center}
  //  \begin{tabular}{|l|c|c|c|c|c|c|c|c|}
  //  \hline
  //  Structure    & Seed Index &  Distance   &   $\sigma$  &     
  //  $\alpha$     &  $\beta$   & Propagation Scaling & Output Image \\  \hline \\ \hline
  //  Left Ventricle  & $(81,114)$ & 5.0 & 1.0 & -0.5 & 3.0  & 2.0 & First  in Figure \ref{fig:GeodesicActiveContourImageFilterOutput2}  \\  \hline
  //  Right Ventricle & $(99,114)$ & 5.0 & 1.0 & -0.5 & 3.0  & 2.0 & Second in Figure \ref{fig:GeodesicActiveContourImageFilterOutput2}  \\  \hline
  //  White matter    & $(56, 92)$ & 5.0 & 1.0 & -0.3 & 2.0  & 10.0 & Third  in Figure \ref{fig:GeodesicActiveContourImageFilterOutput2} \\  \hline
  //  Gray matter     & $(40, 90)$ & 5.0 & 0.5 & -0.3 & 2.0  & 10.0 & Fourth in Figure \ref{fig:GeodesicActiveContourImageFilterOutput2} \\  \hline
  //  \end{tabular}
  //  \end{center}
  //
  //  Figure~\ref{fig:GeodesicActiveContourImageFilterOutput} presents the
  //  intermediate outputs of the pipeline illustrated in
  //  Figure~\ref{fig:GeodesicActiveContoursCollaborationDiagram}. They are
  //  from left to right: the output of the anisotropic diffusion filter, the
  //  gradient magnitude of the smoothed image and the sigmoid of the gradient
  //  magnitude which is finally used as the edge potential for the
  //  \doxygen{GeodesicActiveContourLevelSetImageFilter}.
  //
  // \begin{figure} \center
  // \includegraphics[width=6cm]{BrainProtonDensitySlice.eps}
  // \includegraphics[width=6cm]{GeodesicActiveContourImageFilterOutput1.eps}
  // \includegraphics[width=6cm]{GeodesicActiveContourImageFilterOutput2.eps}
  // \includegraphics[width=6cm]{GeodesicActiveContourImageFilterOutput3.eps}
  // \caption[GeodesicActiveContourLevelSetImageFilter intermediate
  // output]{Images generated by the segmentation process based on the
  // GeodesicActiveContourLevelSetImageFilter. From left to right and top to
  // bottom: input image to be segmented, image smoothed with an
  // edge-preserving smoothing filter, gradient magnitude of the smoothed
  // image, sigmoid of the gradient magnitude. This last image, the sigmoid, is
  // used to compute the speed term for the front propagation }
  // \label{fig:GeodesicActiveContourImageFilterOutput} \end{figure}
  // 
  //  Segmentations of the main brain structures are presented in Figure
  //  ~\ref{fig:GeodesicActiveContourImageFilterOutput2}. The results are quite
  //  similar to those obtained with the \doxygen{ShapeDetectionLevelSetImageFilter}
  //  in section~\ref{sec:ShapeDetectionLevelSetFilter}.
  //
  //  Note that a relatively larger propagation scaling value was
  //  required to segment the white matter. This is due to two
  //  factors: the lower contrast at the border of the white matter
  //  and the complex shape of the structure. Unfortunately the
  //  optimal value of these scaling parameters can only be determined
  //  by experimentation. In a real application we could imagine an
  //  interactive mechanism by which a user supervises the contour
  //  evolution and adjusts these parameters accordingly.
  //
  // \begin{figure} \center
  // \includegraphics[width=4cm]{GeodesicActiveContourImageFilterOutput5.eps}
  // \includegraphics[width=4cm]{GeodesicActiveContourImageFilterOutput6.eps}
  // \includegraphics[width=4cm]{GeodesicActiveContourImageFilterOutput7.eps}
  // \includegraphics[width=4cm]{GeodesicActiveContourImageFilterOutput8.eps}
  // \caption[GeodesicActiveContourImageFilter segmentations]{Images generated by the
  // segmentation process based on the GeodesicActiveContourImageFilter. From left to
  // right: segmentation of the left ventricle, segmentation of the right
  // ventricle, segmentation of the white matter, attempt of segmentation of
  // the gray matter.}
  // \label{fig:GeodesicActiveContourImageFilterOutput2}
  // \end{figure}
  //
  //  Software Guide : EndLatex 



  return 0;

}




