package com.lingavin.gplayer;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import java.io.File;
import java.io.FileFilter;
import java.util.HashMap;

public class MainBrowserActivity extends BaseBrowserActivity
{
    private final static HashMap<String, Class<? extends Activity>> extensionToActivity = new HashMap<String, Class<? extends Activity>>();

    static
    {
        extensionToActivity.put("mp4", GPlayerActivity.class);
        extensionToActivity.put("mov", GPlayerActivity.class);
        extensionToActivity.put("rmvb", GPlayerActivity.class);
        extensionToActivity.put("avi", GPlayerActivity.class);
        extensionToActivity.put("wmv", GPlayerActivity.class);
        extensionToActivity.put("rm", GPlayerActivity.class);
    }

    @Override
    protected FileFilter createFileFilter()
    {
        return new FileFilter()
        {
            public boolean accept(File pathname)
            {
                for (String s : extensionToActivity.keySet())
                {
                    if (pathname.getName().endsWith("." + s)) return true;
                }
                return pathname.isDirectory();
            }
        };
    }

    @Override
    protected void showDocument(Uri uri)
    {
        final Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        String uriString = uri.toString();
        String extension = uriString.substring(uriString.lastIndexOf('.') + 1);
        intent.setClass(this, extensionToActivity.get(extension));
        startActivity(intent);
    }
}
