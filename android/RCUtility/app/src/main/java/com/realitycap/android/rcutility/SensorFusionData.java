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

    public float[] getPose()
    {
        return pose;
    }

    public void setPose(float[] pose)
    {
        this.pose = pose;
    }

    public long getTimestamp()
    {
        return timestamp;
    }

    public void setTimestamp(long timestamp)
    {
        this.timestamp = timestamp;
    }
}
