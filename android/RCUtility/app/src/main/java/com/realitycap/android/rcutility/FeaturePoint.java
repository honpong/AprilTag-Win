package com.realitycap.android.rcutility;

/**
 * Created by benhirashima on 7/17/15.
 */
public class FeaturePoint
{
    protected long id;
    protected Vector3 world;
    protected float imageX;
    protected float imageY;

    public long getId()
    {
        return id;
    }

    public void setId(long id)
    {
        this.id = id;
    }

    public Vector3 getWorld()
    {
        return world;
    }

    public void setWorld(Vector3 world)
    {
        this.world = world;
    }

    public float getImageX()
    {
        return imageX;
    }

    public void setImageX(float imageX)
    {
        this.imageX = imageX;
    }

    public float getImageY()
    {
        return imageY;
    }

    public void setImageY(float imageY)
    {
        this.imageY = imageY;
    }
}
