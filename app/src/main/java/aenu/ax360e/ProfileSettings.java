// SPDX-License-Identifier: WTFPL
package aenu.ax360e;

import android.content.DialogInterface;
import android.os.Bundle;
import android.text.InputFilter;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import java.io.File;
import java.util.ArrayList;

/**
 * Xbox 360 用户档案管理: 列出/创建/重命名/删除档案, 并选择开机自动登录的档案。
 * 档案的磁盘布局与加解密由 native (xe_profile_*) 按 xenia ProfileManager 的
 * 逻辑处理; 开机自动登录写入 xenia-canary.config.toml 的
 * Profiles|logged_profile_slot_0_xuid。
 */
public class ProfileSettings extends AppCompatActivity {

    static final String KEY_AUTO_LOGIN="Profiles|logged_profile_slot_0_xuid";
    static final String DEFAULT_PROFILE_XUID="E0300000A360E000";

    static class Profile{
        final String xuid;
        final String gamertag; //解密失败时为空串
        Profile(String xuid,String gamertag){
            this.xuid=xuid;
            this.gamertag=gamertag;
        }
        String display_name(){
            return gamertag==null||gamertag.isEmpty()? xuid : gamertag;
        }
    }

    interface OnGamertagOk{
        void on_ok(String gamertag);
    }

    ListView list_view;
    final ArrayList<Profile> profiles=new ArrayList<>();
    ProfileAdapter adapter;
    String auto_login_xuid="";

    File content_root(){
        return new File(Application.get_xe_storage_root(),"content");
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);

        Utils.setup_system_bars_appearance(this);
        setContentView(R.layout.activity_profiles);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if(getSupportActionBar()!=null)
            getSupportActionBar().setTitle(getString(R.string.profiles));
        toolbar.setNavigationOnClickListener(v -> onBackPressed());

        if(Emulator.get==null){
            Toast.makeText(this,R.string.loading,Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        list_view=findViewById(R.id.profile_list);
        list_view.setEmptyView(findViewById(R.id.profile_list_is_empty));
        list_view.setOnItemClickListener((l,v,position,id)->
                show_profile_options(position));

        adapter=new ProfileAdapter();
        list_view.setAdapter(adapter);

        load_profiles();
        adapter.notifyDataSetChanged();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu){
        super.onCreateOptionsMenu(menu);
        getMenuInflater().inflate(R.menu.profiles,menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item){
        int item_id=item.getItemId();
        if(item_id==R.id.menu_profile_create){
            show_gamertag_dialog(R.string.profile_create,null, gamertag->{
                if(Emulator.get.xe_profile_create(content_root().getAbsolutePath(),gamertag))
                    refresh();
                else
                    Toast.makeText(this,R.string.profile_op_failed,Toast.LENGTH_SHORT).show();
            });
            return true;
        }
        else if(item_id==R.id.menu_profile_auto_login){
            show_auto_login_dialog();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    void refresh(){
        load_profiles();
        adapter.notifyDataSetChanged();
    }

    void load_profiles(){
        profiles.clear();

        String data=Emulator.get.xe_profile_list(content_root().getAbsolutePath());
        if(data!=null){
            for(String line:data.split("\n")){
                if(line.isEmpty()) continue;
                int tab=line.indexOf('\t');
                if(tab==-1) continue;
                profiles.add(new Profile(line.substring(0,tab),line.substring(tab+1)));
            }
        }

        auto_login_xuid="";
        try{
            File cfg=Application.get_global_config_file();
            if(cfg.exists()){
                Emulator.Config config=Emulator.Config.open_config_file(cfg.getAbsolutePath());
                String v=config.load_config_entry(KEY_AUTO_LOGIN);
                config.close_config_file();
                if(v!=null) auto_login_xuid=v;
            }
        }catch(Exception e){
            e.printStackTrace();
        }
    }

    void show_profile_options(int position){
        final Profile p=profiles.get(position);
        final String[] items={
                getString(R.string.profile_auto_login),
                getString(R.string.profile_rename)
        };
        final String[] item2={
                getString(R.string.profile_auto_login),
                getString(R.string.profile_rename),
                getString(R.string.profile_delete)
        };
        final boolean is_default_profile=p.xuid.equals(DEFAULT_PROFILE_XUID);
        new AlertDialog.Builder(this)
                .setTitle(p.display_name())
                .setItems(is_default_profile? items : item2,(DialogInterface dialog,int which)->{
                    if(which==0)
                        set_auto_login(p.xuid);
                    else if(which==1)
                        show_gamertag_dialog(R.string.profile_rename,p.gamertag,gamertag->{
                            if(Emulator.get.xe_profile_rename(content_root().getAbsolutePath(),p.xuid,gamertag))
                                refresh();
                            else
                                Toast.makeText(this,R.string.profile_op_failed,Toast.LENGTH_SHORT).show();
                        });
                    else
                        confirm_delete(p);
                })
                .show();
    }

    void confirm_delete(final Profile p){
        new AlertDialog.Builder(this)
                .setMessage(getString(R.string.profile_delete_confirm,p.display_name()))
                .setPositiveButton(android.R.string.ok,(DialogInterface dialog,int which)->
                        delete_profile(p))
                .setNegativeButton(android.R.string.cancel,null)
                .show();
    }

    void delete_profile(Profile p){
        if(!Emulator.get.xe_profile_delete(content_root().getAbsolutePath(),p.xuid)){
            Toast.makeText(this,R.string.profile_op_failed,Toast.LENGTH_SHORT).show();
            return;
        }
        if(p.xuid.equals(auto_login_xuid))
            set_auto_login("");
        refresh();
    }

    void set_auto_login(String xuid){
        File cfg=Application.get_global_config_file();
        if(!cfg.exists())
            Utils.copy_file(Application.get_default_config_file(),cfg);
        try{
            Emulator.Config config=Emulator.Config.open_config_file(cfg.getAbsolutePath());
            config.save_config_entry(KEY_AUTO_LOGIN,xuid);
            config.close_config_file();
            auto_login_xuid=xuid;
            adapter.notifyDataSetChanged();
        }catch(Exception e){
            e.printStackTrace();
            Toast.makeText(this,R.string.profile_op_failed,Toast.LENGTH_SHORT).show();
        }
    }

    void show_auto_login_dialog(){
        String[] items=new String[profiles.size()+1];
        int checked=0;
        items[0]=getString(R.string.profile_none);
        for(int i=0;i<profiles.size();i++){
            Profile p=profiles.get(i);
            items[i+1]=p.display_name();
            if(p.xuid.equals(auto_login_xuid))
                checked=i+1;
        }
        new AlertDialog.Builder(this)
                .setTitle(R.string.profile_auto_login)
                .setSingleChoiceItems(items,checked,(DialogInterface dialog,int which)->{
                    set_auto_login(which==0? "" : profiles.get(which-1).xuid);
                    dialog.dismiss();
                })
                .setNegativeButton(android.R.string.cancel,null)
                .show();
    }

    void show_gamertag_dialog(int title_id,String initial,OnGamertagOk ok_cb){
        final EditText edit=new EditText(this);
        edit.setFilters(new InputFilter[]{
                (source,start,end,dest,dstart,dend)->{
                    StringBuilder sb=new StringBuilder();
                    for(int i=start;i<end;i++){
                        char c=source.charAt(i);
                        if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c==' ')
                            sb.append(c);
                    }
                    return sb.toString();
                },
                new InputFilter.LengthFilter(15)
        });
        if(initial!=null) edit.setText(initial);

        int pad=(int)(16*getResources().getDisplayMetrics().density);
        LinearLayout root=new LinearLayout(this);
        root.setPadding(pad,pad,pad,pad);
        root.addView(edit,new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));

        AlertDialog dialog=new AlertDialog.Builder(this)
                .setTitle(title_id)
                .setView(root)
                .setPositiveButton(android.R.string.ok,null)
                .setNegativeButton(android.R.string.cancel,null)
                .create();
        dialog.show();
        //校验失败时保持对话框打开
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
            String gamertag=edit.getText().toString().trim();
            if(!Emulator.is_valid_gamertag(gamertag)){
                Toast.makeText(this,R.string.profile_invalid_gamertag,Toast.LENGTH_SHORT).show();
                return;
            }
            ok_cb.on_ok(gamertag);
            dialog.dismiss();
        });
    }

    class ProfileAdapter extends BaseAdapter{

        @Override
        public int getCount(){
            return profiles.size();
        }

        @Override
        public Object getItem(int position){
            return profiles.get(position);
        }

        @Override
        public long getItemId(int position){
            return position;
        }

        @Override
        public View getView(int position,View curView,ViewGroup parent){
            if(curView==null){
                curView=LayoutInflater.from(ProfileSettings.this)
                        .inflate(R.layout.profile_item,parent,false);
            }
            Profile p=profiles.get(position);

            TextView gamertag=curView.findViewById(R.id.profile_gamertag);
            gamertag.setText(p.gamertag==null||p.gamertag.isEmpty()
                    ? getString(R.string.profile_unreadable) : p.gamertag);

            TextView info=curView.findViewById(R.id.profile_xuid);
            info.setText(p.xuid.equals(auto_login_xuid)
                    ? p.xuid+" · "+getString(R.string.profile_auto_login)
                    : p.xuid);

            return curView;
        }
    }
}
