package com.lingavin.gplayer;

import android.app.Activity;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.TabHost;


import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.List;

import com.lingavin.gplayer.presentation.BrowserAdapter;
import com.lingavin.gplayer.presentation.UriBrowserAdapter;
import com.lingavin.gplayer.R;
import com.viewpagerindicator.TitlePageIndicator;


public abstract class BaseBrowserActivity extends Activity
{
	public static String FILEPATH = Environment.getExternalStorageDirectory().toString();
    private BrowserAdapter adapter;
    private static final String CURRENT_DIRECTORY = "currentDirectory";
    private List<View> viewList;
    private List<String> titleList;
    
    private final AdapterView.OnItemClickListener onItemClickListener = new AdapterView.OnItemClickListener()
    {
        @SuppressWarnings({"unchecked"})
        public void onItemClick(AdapterView<?> adapterView, View view, int i, long l)
        {
            final File file = ((AdapterView<BrowserAdapter>)adapterView).getAdapter().getItem(i);
            if (file.isDirectory())
            {
                setCurrentDir(file);
            }
            else
            {
                showDocument(file);
            }
        }
    };
    private UriBrowserAdapter recentAdapter;
    protected final FileFilter filter;

    public BaseBrowserActivity()
    {
        this.filter = createFileFilter();
    }

    protected abstract FileFilter createFileFilter();

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.browser);
        final ListView browseList = initBrowserListView();
        final ListView recentListView = initRecentListView();
        browseList.setCacheColorHint(Color.TRANSPARENT);
        recentListView.setCacheColorHint(Color.TRANSPARENT);
//        TabHost tabHost = (TabHost) findViewById(R.id.browserTabHost);
//        tabHost.setup();
/*        tabHost.addTab(tabHost.newTabSpec("Browse").setIndicator("Browse").setContent(new TabHost.TabContentFactory()
        {
            public View createTabContent(String s)
            {
                return browseList;
            }
        }));
        tabHost.addTab(tabHost.newTabSpec("Recent").setIndicator("Recent").setContent(new TabHost.TabContentFactory()
        {
            public View createTabContent(String s)
            {
                return recentListView;
            }
        }));*/
        
        viewList = new ArrayList<View>();
        viewList.add(browseList);
        viewList.add(recentListView);
        
        titleList = new ArrayList<String>();
        titleList.add("path");
        titleList.add("recent");
        
        PagerAdapter pagerAdapter = new PagerAdapter(){

			@Override
			public int getCount() {
				return viewList.size();
			}

			@Override
			public boolean isViewFromObject(View arg0, Object arg1) {
				return arg0 == arg1;
			}

			@Override
			public CharSequence getPageTitle(int position) {
				return titleList.get(position);
			}

			@Override
			public Object instantiateItem(ViewGroup container, int position) {
				container.addView(viewList.get(position));
				return viewList.get(position);
			}
			
        };
        
        ViewPager pager = (ViewPager)findViewById(R.id.pager);
        pager.setAdapter(pagerAdapter);
        TitlePageIndicator titleIndicator = (TitlePageIndicator)findViewById(R.id.title);
        titleIndicator.setViewPager(pager);
        titleIndicator.setTextColor(Color.BLUE);
        titleIndicator.setSelectedColor(Color.BLACK);
    }

    @Override
    protected void onPostCreate(Bundle savedInstanceState)
    {
        super.onPostCreate(savedInstanceState);
        final File sdcardPath = new File(FILEPATH);
        if (sdcardPath.exists())
        {
            setCurrentDir(sdcardPath);
        }
        else
        {
            setCurrentDir(new File("/"));
        }
        if (savedInstanceState != null)
        {
            final String absolutePath = savedInstanceState.getString(CURRENT_DIRECTORY);
            if (absolutePath != null)
            {
                setCurrentDir(new File(absolutePath));
            }
        }
    }

    private ListView initBrowserListView()
    {
        final ListView listView = new ListView(this);
        adapter = new BrowserAdapter(this, filter);
        listView.setAdapter(adapter);
        listView.setOnItemClickListener(onItemClickListener);
        listView.setLayoutParams(new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        return listView;
    }

    private ListView initRecentListView()
    {
        ListView listView = new ListView(this);
        recentAdapter = new UriBrowserAdapter();
        listView.setAdapter(recentAdapter);
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
            @SuppressWarnings({"unchecked"})
            public void onItemClick(AdapterView<?> adapterView, View view, int i, long l)
            {
                showDocument(((AdapterView<UriBrowserAdapter>) adapterView).getAdapter().getItem(i));
            }
        });
        listView.setLayoutParams(new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        return listView;
    }

    private void showDocument(File file)
    {
        showDocument(Uri.fromFile(file));
    }

    protected abstract void showDocument(Uri uri);

    private void setCurrentDir(File newDir)
    {
        adapter.setCurrentDirectory(newDir);
        getWindow().setTitle(newDir.getAbsolutePath());
    }

    @Override
    protected void onSaveInstanceState(Bundle outState)
    {
        super.onSaveInstanceState(outState);
        outState.putString(CURRENT_DIRECTORY, adapter.getCurrentDirectory().getAbsolutePath());
    }

    @Override
    protected void onResume()
    {
        super.onResume();
    }
}
