package com.realitycap.android.rcutility;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

/**
 * Created by benhirashima on 8/7/15.
 */
public class TextFileIO
{
    public String readTextFromFileInAssets(String filename)
    {
        InputStream is;
        try
        {
            is = MyApplication.getContext().getAssets().open(filename);
        }
        catch (IOException e)
        {
            Log.e(MyApplication.TAG, e.getLocalizedMessage());
            return null;
        }
        return readTextFromInputStream(is);
    }

    public String readTextFromFileOnSDCard(String filename)
    {
        File file = new File(Environment.getExternalStorageDirectory(), filename);
        FileInputStream is;
        try
        {
            is = new FileInputStream(file);
        }
        catch (FileNotFoundException e)
        {
            Log.e(MyApplication.TAG, e.getLocalizedMessage());
            return null;
        }
        return readTextFromInputStream(is);
    }

    protected String readTextFromInputStream(InputStream is)
    {
        BufferedReader in = null;
        try
        {
            StringBuilder buf = new StringBuilder();
            in = new BufferedReader(new InputStreamReader(is));

            String str;
            while ((str = in.readLine()) != null)
            {
                buf.append(str);
            }
            return buf.toString();
        }
        catch (IOException e)
        {
            Log.e(MyApplication.TAG, e.getLocalizedMessage());
        }
        finally
        {
            if (in != null)
            {
                try
                {
                    in.close();
                }
                catch (IOException e)
                {
                    Log.e(MyApplication.TAG, e.getLocalizedMessage());
                }
            }
        }

        return null;
    }

    public boolean writeTextToFileOnSDCard(String text, String filename)
    {
        FileOutputStream outputStream;
        File file = new File(Environment.getExternalStorageDirectory(), filename);

        try
        {
            outputStream = new FileOutputStream(file);
            outputStream.write(text.getBytes());
            outputStream.close();
        }
        catch (Exception e)
        {
            Log.e(MyApplication.TAG, e.getLocalizedMessage());
            return false;
        }

        return true;
    }
}
