// SPDX-License-Identifier: WTFPL
package aenu.ax360e;

import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

/**
 * Xbox 系统层设置的入口 (Hub): 用户档案 (账号, 每人一份) 与
 * 主机设置 (xconfig, 整机一份) 是并列关系, 从这里分别进入。
 */
public class XboxSettings extends AppCompatActivity {

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);

        Utils.setup_system_bars_appearance(this);
        setContentView(R.layout.activity_xbox_settings);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if(getSupportActionBar()!=null)
            getSupportActionBar().setTitle(getString(R.string.xbox_settings));
        toolbar.setNavigationOnClickListener(v -> onBackPressed());

        ListView list_view = findViewById(R.id.xbox_settings_list);
        list_view.setAdapter(new EntryAdapter());
        list_view.setOnItemClickListener((l,v,position,id)->{
            if(position==0)
                startActivity(new Intent(this,ProfileSettings.class));
            else
                startActivity(new Intent(this,XConfigSettings.class));
        });
    }

    class EntryAdapter extends BaseAdapter {

        final int[] titles={
                R.string.profiles,
                R.string.console_settings
        };
        final int[] subtitles={
                R.string.xbox_settings_profiles_hint,
                R.string.xbox_settings_console_hint
        };

        @Override
        public int getCount(){
            return titles.length;
        }

        @Override
        public Object getItem(int position){
            return null;
        }

        @Override
        public long getItemId(int position){
            return position;
        }

        @Override
        public View getView(int position,View curView,ViewGroup parent){
            if(curView==null){
                curView=LayoutInflater.from(XboxSettings.this)
                        .inflate(R.layout.xbox_settings_item,parent,false);
            }
            ((TextView)curView.findViewById(R.id.entry_title)).setText(titles[position]);
            ((TextView)curView.findViewById(R.id.entry_subtitle)).setText(subtitles[position]);
            return curView;
        }
    }
}
