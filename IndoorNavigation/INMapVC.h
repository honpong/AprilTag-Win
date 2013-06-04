
#import <GLKit/GLKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreImage/CoreImage.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreLocation/CoreLocation.h>
#import "RCCore/RCAVSessionManagerFactory.h"
#import "RCCore/RCCorvisManagerFactory.h"
#import "RCCore/RCMotionCapManagerFactory.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCLocationManagerFactory.h"
#import "MBProgressHUD.h"
#import "RCCore/RCConstants.h"
#import "RCCore/feature_info.h"
#import "RCCore/RCDistanceLabel.h"
#import "RCCore/RCCalibration.h"
#import <MapKit/MapKit.h>

#define FEATURE_COUNT 80
#define VIDEO_WIDTH 480
#define VIDEO_HEIGHT 640

@interface INMapVC : UIViewController <MKMapViewDelegate>

- (void)handlePause;
- (void)handleResume;
- (void)startNavigating;
- (void)stopNavigating;
- (IBAction)handleLocationButton:(id)sender;

void TMNewMeasurementVCUpdateMeasurement(void *self, float x, float stdx, float y, float stdy, float z, float stdz, float path, float stdpath, float rx, float stdrx, float ry, float stdry, float rz, float stdrz);

@property (weak, nonatomic) IBOutlet MKMapView *mapView;
@property (weak, nonatomic) IBOutlet UIImageView *arrowImage;
@property (weak, nonatomic) IBOutlet UIImageView *statusIcon;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (weak, nonatomic) IBOutlet UIView *instructionsBg;

@end
