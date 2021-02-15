/*
This sample illustrates how to get audio data from an input device, such as a microphone,
with audio::InputDeviceNode. It then visualizes the input in the frequency domain. The frequency
spectrum analysis is accomplished with an audio::MonitorSpectralNode.
The plot is similar to a typical spectrogram, where the x-axis represents the linear
frequency bins (0 - samplerate / 2) and the y-axis is the magnitude of the frequency
bin in normalized decibels (0 - 100).
author: Richard Eakin (2014)
*/

/*
This example uses InputAnalyzer (author: Richard Eakin, 2014) and modifies some of the variables to read pitch.
The goal is to access frequency bins, and to read the dominant frequency, based on placement of the MonitorSpectralNode.
This can be used for apps that generate visuals based on audio-reactivity.
For example, if frequency ranges between 80 and 200 are detected and volume is higher than 0 - a circle will appear.
author: Tom Estlack 2021
 */

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/audio/audio.h"
#include "../../common/AudioDrawUtils.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class InputAnalyzer : public App {
  public:
    void setup() override;
    void mouseDown( MouseEvent event ) override;
    void update() override;
    void draw() override;

    void drawSpectralCentroid();
    void drawLabels();
    void printBinInfo( int mouseX );

    audio::InputDeviceNodeRef        mInputDeviceNode;
    audio::MonitorSpectralNodeRef    mMonitorSpectralNode;
    vector<float>                    mMagSpectrum;

    SpectrumPlot                    mSpectrumPlot;
    gl::TextureFontRef                mTextureFont;

};

void InputAnalyzer::setup()
{
    auto ctx = audio::Context::master();
    // The InputDeviceNode is platform-specific, so you create it using a special method on the Context:
    mInputDeviceNode = ctx->createInputDeviceNode();
    // By providing an FFT size double that of the window size, we 'zero-pad' the analysis data, which gives
    // an increase in resolution of the resulting spectrum data.
    auto monitorFormat = audio::MonitorSpectralNode::Format().fftSize( 2048 ).windowSize( 1024 );
    mMonitorSpectralNode = ctx->makeNode( new audio::MonitorSpectralNode( monitorFormat ) );
    mInputDeviceNode >> mMonitorSpectralNode;
    // InputDeviceNode (and all InputNode subclasses) need to be enabled()'s to process audio. So does the Context:
    mInputDeviceNode->enable();
    ctx->enable();
    getWindow()->setTitle( mInputDeviceNode->getDevice()->getName() );
}

void InputAnalyzer::mouseDown( MouseEvent event )
{
    if( mSpectrumPlot.getBounds().contains( event.getPos() ) )
        printBinInfo( event.getX() );
}

void InputAnalyzer::update()
{
    //  changed from InputAnalyzer - window dimensions to be set at 1024 x 768 for consistent readings
    mSpectrumPlot.setBounds( Rectf( 40, 40, (float)1024 - 40, (float)768 - 40 ) );
    // We copy the magnitude spectrum out from the Node on the main thread, once per update:
    mMagSpectrum = mMonitorSpectralNode->getMagSpectrum();
}

void InputAnalyzer::draw()
{
    gl::clear();
    gl::enableAlphaBlending();
    mSpectrumPlot.draw( mMagSpectrum );
    drawSpectralCentroid();
    drawLabels();
}

void InputAnalyzer::drawSpectralCentroid()
{
    // The spectral centroid is largely correlated with 'brightness' of a sound. It is the center of mass of all frequency values.
    // See the note on audio::MonitorSpectralNode::getSpectralCentroid() - it may be analyzing a more recent magnitude spectrum
    // than what we're drawing in the SpectrumPlot. It is not a problem for this simple sample, but if you need a more precise
    // value, use audio::dsp::spectralCentroid() directly.
    float spectralCentroid = mMonitorSpectralNode->getSpectralCentroid();
    float nyquist = (float)audio::master()->getSampleRate() / 2.0f;
    Rectf bounds = mSpectrumPlot.getBounds();
    // revised variable MyQuisp - .745 ended up being a "sweet spot" but is off by roughly 10-4 hz
    // ie. low e on guitar is 82hz, reports as 86hz - high e is 322hz, reports as 362hz (or something)
    float MyQuisp = (float)audio::master()->getSampleRate() / 0.745f;
    float frNormT = spectralCentroid / MyQuisp; // Needed to read freq
    float freqNormalized = spectralCentroid / nyquist;
    float barCenter = bounds.x1 + freqNormalized * bounds.getWidth();
    // try to read frequencies -> Eakin
    Rectf verticalBar = { barCenter - 2, bounds.y1, barCenter + 2, bounds.y2 };
    gl::ScopedColor colorScope( 0.85f, 0.45f, 0, 0.4f ); // transparent orange
    gl::drawSolidRect( verticalBar );
    
    // locate frequency bin with somewhat better accuracy to measure frequency;
    float FBins = (bounds.x1 + frNormT * 1024) - 40;
    // snag frequency using location of spectral node (above) as coord for bin#
    float FCalc = mMonitorSpectralNode->getFreqForBin(FBins);// was FBins
    // might need to correct since bounds could change dep on scr size
    // measure volume mag of bin# (where dominant frequency is located
    float FVolm = audio::linearToDecibel( mMagSpectrum[FBins] );
    
    gl::color(1,1,1);
    gl::drawSolidCircle(vec2(FBins, FVolm), 50); // follows bin location
    /* uncomment to see measurements
     if (FVolm > 0) {
        console() << "FCalc-" << FCalc << "|vol-" << FVolm << "|FBins-" << FBins << " ";
    }
     */
    // low e and mid a guitar
    if ((FCalc > 200) && (FCalc < 400) && (FVolm > 10)) {
        gl::color(1,0,0);
        gl::drawSolidCircle(vec2(getWindowCenter().x,getWindowCenter().y*.5), FVolm);
    }
    // mid a and high a
    if ((FCalc < 200)  && (FVolm > 10)) {
        gl::color(0,1,0);
        gl::drawSolidCircle(vec2(getWindowCenter().x*.5,getWindowCenter().y*.5), FVolm);
    }
    // high a and way up there
    if ((FCalc > 400)  && (FVolm > 10)) {
        gl::color(0,0,1);
        gl::drawSolidCircle(vec2(getWindowCenter().x*1.5,getWindowCenter().y*.5), FVolm);
    }

    // frequency reference
    // human hearing 20hz 20000hz - only need range of 30 - 5000
    // bass 5 string 31hz - 262hz
    // guitar 82hz - 1379hz
    // drums 60z (kick) - 5000hz (hi hat)
    
    // float freqDetect = mMonitorSpectralNode->getFreqForBin(250);
    //gl::clear();
    gl::color(Color(1.0f, 0.0f, .7f));
    //gl::drawSolidRect(Rectf(0,0,200,200) );
    gl::drawSolidCircle(getWindowCenter(), spectralCentroid/200);
    gl::color(Color(0, 1.0f, .5f));
    gl::drawSolidCircle(vec2(getWindowCenter().x*1.5f,getWindowCenter().y), spectralCentroid/300);
    gl::color(Color(0,spectralCentroid/10000,1.0f));
    gl::drawSolidCircle(vec2(getWindowCenter().x*0.5f,getWindowCenter().y), 100);
}

void InputAnalyzer::drawLabels()
{
    if( ! mTextureFont )
        mTextureFont = gl::TextureFont::create( ci::Font( ci::Font::getDefault().getName(), 16 ) );

    gl::color( 0, 0.9f, 0.9f );

    // draw x-axis label
    string freqLabel = "Frequency (hertz)";
    mTextureFont->drawString( freqLabel, vec2( getWindowCenter().x - mTextureFont->measureString( freqLabel ).x / 2, (float)getWindowHeight() - 20 ) );

    // draw y-axis label
    string dbLabel = "Magnitude (decibels, linear)";
    gl::pushModelView();
        gl::translate( 30, getWindowCenter().y + mTextureFont->measureString( dbLabel ).x / 2 );
        gl::rotate( -M_PI / 2 );
        mTextureFont->drawString( dbLabel, vec2( 0 ) );
    gl::popModelView();
    
}

void InputAnalyzer::printBinInfo( int mouseX )
{
    size_t numBins = mMonitorSpectralNode->getFftSize() / 2;
    size_t bin = std::min( numBins - 1, size_t( ( numBins * ( mouseX - mSpectrumPlot.getBounds().x1 ) ) / mSpectrumPlot.getBounds().getWidth() ) );

    float binFreqWidth = mMonitorSpectralNode->getFreqForBin( 1 ) - mMonitorSpectralNode->getFreqForBin( 0 );
    float freq = mMonitorSpectralNode->getFreqForBin( bin );
    float mag = audio::linearToDecibel( mMagSpectrum[bin] );

    console() << "bin: " << bin << ", freqency (hertz): " << freq << " - " << freq + binFreqWidth << ", magnitude (decibels): " << mag << endl;
}

CINDER_APP( InputAnalyzer, RendererGl( RendererGl::Options().msaa( 8 ) ) )
