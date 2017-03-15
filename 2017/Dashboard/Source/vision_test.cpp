#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv/cv.h"

cv::Mat process(cv::Mat frame)
{	
	cv::Scalar low(120, 100, 16);
	cv::Scalar high(255, 255, 64);
	
	cv::Mat masked;
	cv::inRange(frame, low, high, masked);
	
	cv::Mat eroded;
	cv::erode(masked, eroded, cv::Mat());
	
	cv::Mat dilated;
	cv::dilate(eroded, dilated, cv::Mat());
	
	return dilated;
}

bool sort_by_size(cv::Rect a, cv::Rect b)
{
	return (a.area() > b.area());
}

r32 Absolute(r32 a)
{
	return (a > 0.0f) ? a : -a;
}

struct Target
{
	b32 hit;
	cv::Rect top_box;
	cv::Rect bottom_box;
};

Target track(cv::Mat masked)
{
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(masked.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	std::vector<cv::Rect> hits;
	
	for(u32 i = 0; i < contours.size(); i++)
	{
		cv::Rect contour = cv::boundingRect(contours[i]);
		if(contour.area() > 40)
		{
			hits.push_back(contour);
		}
	}
	
	Target result = {};
	
	if(hits.size() >= 2)
	{
		std::sort(hits.begin(), hits.end(), sort_by_size);
		
		r32 box0_y = hits[0].y + hits[0].height / 2;
		r32 box1_y = hits[1].y + hits[1].height / 2;
		
		cv::Rect top_box = (box0_y > box1_y) ? hits[0] : hits[1];
		cv::Rect bottom_box = (box0_y > box1_y) ? hits[1] : hits[0];
		
		r32 difference = (top_box.x + top_box.width / 2) - (bottom_box.x + bottom_box.width / 2);
		
		if(Absolute(difference) < 10)
		{	
			result.hit = true;
			result.top_box = top_box;
			result.bottom_box = bottom_box;
		}
	}
	
	return result;
}

r32 VisionTest(cv::VideoCapture cap, s32 brightness,
			   cv::Rect top_reference, cv::Rect bottom_reference)
{
	cv::Mat frame;
	bool frame_success = cap.read(frame);
		
	if(frame_success)
	{
		frame = frame + cv::Scalar(brightness, brightness, brightness);
		cv::Mat masked = process(frame);
		Target target = track(masked);
		
		if(target.hit)
		{
			r32 top_movement = (top_reference.x + top_reference.width / 2) - (target.top_box.x + target.top_box.width / 2);
			r32 bottom_movement = (bottom_reference.x + bottom_reference.width / 2) - (target.bottom_box.x + target.bottom_box.width / 2);
				
			r32 movement = (top_movement + bottom_movement) / 2;
			return movement;
		}
	}
	
	return 0;
}