
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
#import "RCCore/RCPimManagerFactory.h"
#import "RCCore/RCMotionCapManagerFactory.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCLocationManagerFactory.h"
#import "MBProgressHUD.h"
#import "RCCore/feature_info.h"
#import "RCCore/RCDistanceLabel.h"
#import "RCCore/RCCalibration.h"
#import <GoogleMaps/GoogleMaps.h>
#import "RCCore/RCMeasurementManager.h"

@interface INMapVC : UIViewController <CLLocationManagerDelegate, RCMeasurementManagerDelegate>

- (void)handlePause;
- (void)handleResume;
- (void)startNavigating;
- (void)stopNavigating;
- (IBAction)handleLocationButton:(id)sender;
- (void) locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading;

@property (weak, nonatomic) GMSMapView *mapView;
@property (weak, nonatomic) IBOutlet UIImageView *arrowImage;
@property (weak, nonatomic) IBOutlet UIImageView *statusIcon;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (weak, nonatomic) IBOutlet UIView *instructionsBg;

@end
