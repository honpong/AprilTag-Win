package com.realitycap.android.rcutility;

import com.intel.camera.toolkit.depth.Point3DF;

import java.util.ArrayList;

public class SensorFusionData
{
    protected long timestamp;
    protected float[] pose;
    protected ArrayList<FeaturePoint> features;

    SensorFusionData ()
    {
        features = new ArrayList<>();
        pose = new float[12];
    }

    public ArrayList<FeaturePoint> getFeatures()
    {
        return features;
    }

    public void addFeaturePoint(long id, float worldX, float worldY, float worldZ, float imageX, float imageY)
    {
        Point3DF world = new Point3DF();
        world.x = worldX;
        world.y = worldY;
        world.z = worldZ;

        FeaturePoint feature = new FeaturePoint();
        feature.id = id;
        feature.world = world;
        feature.imageX = imageX;
        feature.imageY = imageY;

        features.add(feature);
    }

    public void clearFeaturePoints()
    {
        features.clear();
    }

    public float[] getPose()
    {
        return pose;
    }

    public void setPose(float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11)
    {
        pose[0] = f0;
        pose[1] = f1;
        pose[2] = f2;
        pose[3] = f3;
        pose[4] = f4;
        pose[5] = f5;
        pose[6] = f6;
        pose[7] = f7;
        pose[8] = f8;
        pose[9] = f9;
        pose[10] = f10;
        pose[11] = f11;
    }

    public long getTimestamp()
    {
        return timestamp;
    }

    public void setTimestamp(long timestamp)
    {
        this.timestamp = timestamp;
    }

    public String toPoseString()
    {
        StringBuilder sb = new StringBuilder(pose.length + 2);
        sb.append("pose [");
        boolean isFirst = true;
        for (int i = 0; i < pose.length; i++)
        {
            if (isFirst)
                isFirst = false;
            else
                sb.append(",");
            sb.append(String.format("%.2f", pose[i]));
        }
        sb.append("]");
        return sb.toString();
    }
}
