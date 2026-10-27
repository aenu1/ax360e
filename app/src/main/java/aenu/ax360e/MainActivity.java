package aenu.ax360e;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ShortcutInfo;
import android.content.pm.ShortcutManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.preference.PreferenceManager;
import android.provider.DocumentsContract;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.documentfile.provider.DocumentFile;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.w3c.dom.Text;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Base64;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class MainActivity extends AppCompatActivity {

    public static final int REQUEST_SELECT_GAME_DIR=6004;
    static final int DELAY_ON_CREATE=0xaeae0000;
    public static final String PREF_GAME_DIR="game_dir";
    public static File get_game_list_file(){
        return new File(Application.get_app_data_dir(),"game_list.json");
    }


    private final AdapterView.OnItemClickListener item_click_l=new AdapterView.OnItemClickListener(){
        @Override
        public void onItemClick(AdapterView<?> l, View v, int position,long id)
        {

            Emulator.GameInfo meta_info=((GameMetaInfoAdapter)l.getAdapter()).getMetaInfo(position);

            Intent intent = new Intent("aenu.intent.action.AX360E");
            intent.setPackage(getPackageName());

            intent.putExtra(EmulatorActivity.EXTRA_GAME_URI,meta_info.uri);

            File custom_config_file=Application.get_custom_config(meta_info.title_id);
            if(custom_config_file.exists()){
                intent.putExtra(EmulatorActivity.EXTRA_CONFIG_PATH, custom_config_file.getAbsolutePath());
            }

            startActivity(intent);
        }
    };

    ListView list_view;
    ProgressBar progress;
    ProgressTask progress_task;
    Emulator.Config config;
    Dialog delay_dialog=null;
    final Handler delay_on_create=new Handler(new Handler.Callback() {
        @Override
        public boolean handleMessage(@NonNull Message msg) {
            if(msg.what!=DELAY_ON_CREATE) return false;
            if(delay_dialog!=null){
                delay_dialog.dismiss();
                delay_dialog=null;
            }
            on_create();
            return true;
        }
    });

    void show_device_unsupport_vulkan_dialog(){

        AlertDialog.Builder ab=new AlertDialog.Builder(this);
        ab.setPositiveButton(R.string.quit, new DialogInterface.OnClickListener(){

            @Override
            public void onClick(DialogInterface p1, int p2)
            {
                p1.cancel();
                finish();
            }


        });
        Dialog d=ab.create();
        d.setCanceledOnTouchOutside(false);
        d.setOnKeyListener(new DialogInterface.OnKeyListener(){
            @Override
            public boolean onKey(DialogInterface p1, int p2, KeyEvent p3){
                return true;
            }
        });
        d.show();
    }

    void _on_create(){
        /*Emulator.get.setup_context(this);
        //FIXME
        Emulator.get.setup_document_file_tree(DocumentFile.fromTreeUri(this,MainActivity.load_pref_game_dir( this)));
        Emulator.get.setup_launch_args(new String[]{
                "--storage_root=/storage/emulated/0/Download/ax360e",
                "--log_file=/storage/emulated/0/Download/ax360e/xe.log",
        });*/
        setContentView(R.layout.activity_main);
        
        // Setup Toolbar
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if(getSupportActionBar() != null) {
            getSupportActionBar().setTitle(R.string.app_name);
        }
        
        list_view=findViewById(R.id.game_list);
        list_view.setOnItemClickListener(item_click_l);
        list_view.setEmptyView(findViewById(R.id.game_list_is_empty));

        registerForContextMenu(list_view);
        //refresh_game_list();
        show_game_list();
        /*if(!new File(Application.get_app_data_dir(),"xenia-canary.config.toml").exists()) return;
        String config_str=Emulator.get.generate_config_xml(new File(Application.get_app_data_dir(),"xenia-canary.config.toml").getAbsolutePath());
        File config_file=new File(Application.get_app_data_dir(),"xenia.config.xml");
        Utils.save_string(config_file,config_str);*/
    }

    void on_create(){com.google.android.gms.ads.MobileAds.initialize(this, initializationStatus -> {
final AppOpenAdManager app_open_ad_manager=AppOpenAdManager.getInstance(this);
app_open_ad_manager.loadAd(this, new AppOpenAdManager.OnLoadAdCompleteListener() {
@Override
public void onLoadAdComplete() {
app_open_ad_manager.showAdIfAvailable( MainActivity.this, new AppOpenAdManager.OnShowAdCompleteListener() {
@Override
public void onShowAdComplete() {
_on_create();
}});}});});}


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);

        Utils.setup_system_bars_appearance(this);

        if(!Application.device_support_vulkan()){
            show_device_unsupport_vulkan_dialog();
            return;
        }

        if(!Application.should_delay_load()){
            on_create();
            return;
        }

        delay_dialog=ProgressTask.create_progress_dialog( this,getString(R.string.loading));
        delay_dialog.show();
        new Thread(){
            @Override
            public void run() {
                try {
                    Thread.sleep(500);
                    Emulator.load_library();
                    Thread.sleep(100);
                    delay_on_create.sendEmptyMessage(DELAY_ON_CREATE);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }
        }.start();
    }

    @Override
    protected void onStart() {super.onStart();
AppOpenAdManager.getInstance(this).showAdIfAvailable( this);}

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(progress_task!=null){
            progress_task.force_close();
            progress_task=null;
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu){
        super.onCreateOptionsMenu(menu);
        getMenuInflater().inflate(R.menu.main,menu);
        return true;
    }
    @Override
    public boolean onKeyDown(int keyCode,KeyEvent event){
        if(keyCode==KeyEvent.KEYCODE_BACK){
            finish();
            return true;
        }
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            openOptionsMenu();
            return true;
        }
        Toolbar toolbar = findViewById(R.id.toolbar);

        if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
            ListView listView = findViewById(R.id.game_list);
            if (listView != null && listView.getSelectedItemPosition() <= 0) {
                if (toolbar != null && toolbar.getChildCount() > 0) {
                    View lastChild = toolbar.getChildAt(toolbar.getChildCount() - 1);
                    if (lastChild instanceof ViewGroup) {
                        ViewGroup group = (ViewGroup) lastChild;
                        if (group.getChildCount() > 0) {
                            View lastMenuItem = group.getChildAt(group.getChildCount() - 1);
                            lastMenuItem.setFocusable(true);
                            lastMenuItem.setFocusableInTouchMode(true);
                            lastMenuItem.requestFocus();
                            return true;
                        }
                    }
                }
            }
        }
        if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
            if (toolbar != null && toolbar.hasFocus()) {
                ListView listView = findViewById(R.id.game_list);
                if (listView != null) {
                    listView.requestFocus();
                    listView.setSelection(0);
                    return true;
                }
            }
        }

        return super.onKeyDown(keyCode,event);
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);
        AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
        getMenuInflater().inflate(R.menu.game_options, menu);
        if(!getPackageName().equals("aenu.ax360e"))
            menu.removeItem(R.id.create_shortcut);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();

        int position = info.position;
        int item_id = item.getItemId();
        if(item_id==R.id.create_shortcut){
            ShortcutManager shortcutManager=getSystemService(ShortcutManager.class);
            Emulator.GameInfo meta_info=adapter.getMetaInfo(position);
            Bitmap icon;
            if(meta_info.icon!=null)
                icon=BitmapFactory.decodeByteArray(meta_info.icon,0,meta_info.icon.length);
            else
                icon=BitmapFactory.decodeResource(getResources(),R.drawable.app_icon);

            Intent intent=new Intent(this,EmulatorActivity.class);
            {
                intent.setAction(Intent.ACTION_VIEW);
                intent.putExtra(EmulatorActivity.EXTRA_GAME_URI, meta_info.uri);
                File custom_config_file=Application.get_custom_config(meta_info.title_id);
                if(custom_config_file.exists()){
                    intent.putExtra(EmulatorActivity.EXTRA_CONFIG_PATH, custom_config_file.getAbsolutePath());
                }
            }
            shortcutManager.requestPinShortcut(new ShortcutInfo.Builder(this, meta_info.name)
                    .setShortLabel(meta_info.name)
                    .setIcon(Icon.createWithBitmap( icon))
                    .setIntent(intent)
                    .build(), null);
        }
        else if(item_id==R.id.game_patches){
            Emulator.GameInfo meta_info=adapter.getMetaInfo(position);
            show_game_patch_dialog(meta_info);
        }
        else if(item_id==R.id.game_info){
            Emulator.GameInfo meta_info=adapter.getMetaInfo(position);
            show_game_info_dialog(meta_info);
        }
        else if(item_id==R.id.edit_custom_config){
            Intent intent=new Intent(this,EmulatorSettings.class);
            File cfg_file=Application.get_custom_config(adapter.getMetaInfo(position).title_id);
            if(!cfg_file.exists()){
                cfg_file.getParentFile().mkdirs();
                Utils.copy_file(Application.get_global_config_file(),cfg_file);
            }
            intent.putExtra(EmulatorSettings.EXTRA_CONFIG_PATH, cfg_file.getAbsolutePath());
            startActivity(intent);
        }

        return super.onContextItemSelected(item);
    }


    static Intent get_file_manager_intent(String pkg_name)
    {
        Intent it=new Intent(Intent.ACTION_VIEW);
        it.setClassName(pkg_name, "com.android.documentsui.files.FilesActivity");
        it.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return it;
    }

    static void open_file_manager(Activity activity)
    {
        try{
            activity.startActivity(get_file_manager_intent("com.android.documentsui"));
            return;
        }
        catch(Exception e){
        }
        try{
            activity.startActivity(get_file_manager_intent("com.google.android.documentsui"));
            return;
        }
        catch(Exception e){
        }
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item){

        int item_id=item.getItemId();
        if(item_id==R.id.menu_refresh_list){
            refresh_and_show_game_list();
            return true;
        }
        else if(item_id==R.id.menu_key_mappers){
            startActivity(new Intent(this,KeyMapActivity.class));
            return true;
        }
        else if(item_id==R.id.menu_set_game_dir){
            request_game_dir_select(this);
            return true;
        }
        else if(item_id==R.id.menu_open_file_mgr){
            open_file_manager( this);
            return true;
        }
        else if(item_id==R.id.menu_about){
            startActivity(new Intent(this,AboutActivity.class));
        }
        else if(item_id==R.id.menu_settings){
            startActivity(new Intent(this,EmulatorSettings.class));
            return true;
        }
        else if(item_id==R.id.menu_xbox_settings){
            startActivity(new Intent(this,XboxSettings.class));
            return true;
        }
        else if(item_id==R.id.menu_virtual_pad_edit){
            startActivity(new Intent(this,VirtualControlEdit.class));
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null) return;

        Uri uri = data.getData();

        if(requestCode == REQUEST_SELECT_GAME_DIR){
            save_pref_game_dir(this,uri);
            refresh_and_show_game_list();
            return;
        }
    }
    static void request_game_dir_select(Activity activity){
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        activity.startActivityForResult(intent, REQUEST_SELECT_GAME_DIR);
    }
    GameMetaInfoAdapter adapter=null;
    private void refresh_and_show_game_list(){

        (progress_task=new ProgressTask( this)
                .set_progress_message(getString( R.string.game_list_loading))
                .set_done_task(new ProgressTask.UI_Task() {
                    @Override
                    public void run() {
                        show_game_list();
                        progress_task=null;
                    }
                }))
                .call( new ProgressTask.Task() {
                    @Override
                    public void run(ProgressTask task) {
                        ArrayList<Emulator.GameInfo> metas=GameMetaInfoAdapter.refresh_game_list( MainActivity.this);

                        File json_file=get_game_list_file();
                        if(json_file.exists())
                            json_file.delete();
                        GameMetaInfoAdapter.save_game_list_to_json_file(json_file,GameMetaInfoAdapter.refresh_game_list(MainActivity.this));
                        task.task_handler.sendEmptyMessage(ProgressTask.TASK_DONE);
                    }
                });
        //show_game_list();
    }

    private void show_game_list(){

        try{
            ArrayList<Emulator.GameInfo> metas=GameMetaInfoAdapter.load_game_list_from_json_file(get_game_list_file());
            adapter=new GameMetaInfoAdapter(this,metas);
            ((ListView)findViewById(R.id.game_list)).setAdapter(adapter);
        }
        catch(Exception e){
            refresh_and_show_game_list();
        }
    }

    void show_game_info_dialog(Emulator.GameInfo meta_info)
    {
        AlertDialog.Builder builder=new AlertDialog.Builder(this);
        builder.setTitle("["+meta_info.title_id+"]"+meta_info.name);
        builder.setMessage(meta_info.uri.toString());
        builder.setPositiveButton(android.R.string.ok,null);
        builder.show();
    }

    void show_game_patch_dialog(Emulator.GameInfo meta_info){

        Emulator.PatchManager.PatchContext patchContext = Emulator.PatchManager.getPatchContextForGame(meta_info.title_id);
        if(patchContext == null){
            Toast.makeText(this, R.string.patches_no_match, Toast.LENGTH_SHORT).show();
            return;
        }

        String[] patchNames = new String[patchContext.patches.size()];
        boolean[] checkedStates = new boolean[patchNames.length];

        for (int i = 0; i < patchNames.length; i++) {
            Emulator.PatchManager.PatchInfo patch = patchContext.patches.get(i);
            patchNames[i] = patch.name;
            checkedStates[i] = patch.isEnabled;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(meta_info.name)
                .setMultiChoiceItems(patchNames, checkedStates, new DialogInterface.OnMultiChoiceClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which, boolean isChecked) {
                        checkedStates[which] = isChecked;
                    }
                })
                .setPositiveButton(getString(R.string.apply), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                        for (int i = 0; i < checkedStates.length; i++) {
                            Emulator.PatchManager.updatePatchStatus(patchContext, patchContext.patches.get(i), checkedStates[i]);
                        }
                        Emulator.PatchManager.savePatchFile(patchContext);
                        Toast.makeText(MainActivity.this, getString(R.string.patch_settings_saved), Toast.LENGTH_SHORT).show();
                    }
                })
                .setNegativeButton(android.R.string.cancel, null)
                .create().show();
    }

    static void save_pref_game_dir(Context ctx,Uri uri){
        try{
            ctx.getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);
            SharedPreferences.Editor editor= PreferenceManager.getDefaultSharedPreferences(ctx).edit();
            editor.putString(PREF_GAME_DIR,uri.toString());
            editor.apply();
        }
        catch(Exception e){
            e.printStackTrace();
        }
    }

    static Uri load_pref_game_dir(Context ctx){
        try{
            String uri_str=PreferenceManager.getDefaultSharedPreferences(ctx).getString(PREF_GAME_DIR,null);
            if(uri_str==null)
                return null;
            Uri uri= Uri.parse(uri_str);
            ctx.getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);
            return uri;
        }
        catch(Exception e){
            e.printStackTrace();
            return null;
        }
    }
    private static class GameMetaInfoAdapter extends BaseAdapter {

        private static class Filter{

            static boolean is_god_game(String file_name){
                /*int file_name_len = file_name.length();
                if(file_name_len == 20||file_name_len==42) {
                    final String HEX_CHARS = "0123456789ABCDEF";
                    for(int i = 0; i < file_name_len; i++){
                        char c = file_name.charAt(i);
                        if(HEX_CHARS.indexOf(c) == -1) return false;
                    }
                    return true;
                }
                else return false;*/
                return file_name.indexOf('.')==-1;
            }

            static boolean is_iso_file(String file_name){
                return file_name.endsWith(".iso");
            }

            static boolean is_zar_file(String file_name){
                return file_name.endsWith(".zar");
            }

            static DocumentFile get_default_xex_file(DocumentFile dir){
                DocumentFile[] files=dir.listFiles();
                if(files == null) return null;
                if(files.length == 0) return null;
                for(DocumentFile file:files){
                    if(!file.isFile()) continue;
                    if(file.getName().toLowerCase().equals("default.xex")) return file;
                }
                return null;
            }

            static boolean is_xex_game(DocumentFile dir){
                return get_default_xex_file(dir) != null;
            }
        };

        ArrayList<Emulator.GameInfo> metas;
        private final MainActivity context_;
        private GameMetaInfoAdapter(MainActivity context,ArrayList<Emulator.GameInfo> metas){
            context_=context;
            this.metas=metas;
        }

        static String load_file_as_string(File file) throws IOException {
            FileInputStream fis=new FileInputStream(file);
            ByteArrayOutputStream baos=new ByteArrayOutputStream();
            byte[] buffer=new byte[16384];
            int n;
            while ((n=fis.read(buffer))!=-1){
                baos.write(buffer,0,n);
            }
            fis.close();
            return baos.toString();
        }
        static ArrayList<Emulator.GameInfo> load_game_list_from_json_file(File json) throws JSONException, IOException {
            String json_str=load_file_as_string(json);
            return load_game_list_from_json(json_str);
        }

        static ArrayList<Emulator.GameInfo> load_game_list_from_json(String json_str) throws JSONException {
            ArrayList<Emulator.GameInfo> metas=new ArrayList<Emulator.GameInfo>();
            JSONArray game_list=new JSONArray(json_str);
            for(int i=0;i<game_list.length();i++){
                JSONObject game_info=game_list.getJSONObject(i);
                Emulator.GameInfo meta=Emulator.GameInfo.from_json(game_info);
                metas.add(meta);
            }
            return metas;
        }

        static void save_game_list_to_json_file(File json,ArrayList<Emulator.GameInfo> metas){
            try {
                FileOutputStream fos=new FileOutputStream(json);
                fos.write(save_game_list_to_json(metas).getBytes());
                fos.close();
            } catch (IOException | JSONException e) {
                e.printStackTrace();
            }
        }

        static String save_game_list_to_json(ArrayList<Emulator.GameInfo> metas) throws JSONException {
            JSONArray game_list=new JSONArray();
            for(Emulator.GameInfo meta:metas){
                game_list.put(Emulator.GameInfo.to_json(meta));
            }
            return game_list.toString();
        }



        static ArrayList<Emulator.GameInfo> refresh_game_list(MainActivity context){
            ArrayList<Emulator.GameInfo> metas=new ArrayList<Emulator.GameInfo>();
            Uri uri=context.load_pref_game_dir(context);
            if(uri==null)
                return metas;
            DocumentFile iso_dir=DocumentFile.fromTreeUri(context, uri);
            if(iso_dir==null||!iso_dir.exists())
                return metas;
            DocumentFile[] files=iso_dir.listFiles();
            for(DocumentFile file:files){
                if(file.isDirectory()){

                    DocumentFile default_xex_file=Filter.get_default_xex_file(file);
                    if(default_xex_file==null) continue;
                    Emulator.GameInfo meta=Emulator.get.meta_info_from_xex_game(context,default_xex_file.getUri().toString());
                    if(meta!=null){
                        if(meta.name== null){
                            meta.name=file.getName();
                        }
                        metas.add(meta);
                    }
                }
                else{
                    if(Filter.is_iso_file(file.getName())){
                        Emulator.GameInfo meta=Emulator.get.meta_info_from_iso_game(context,file.getUri().toString());
                        if(meta!=null) metas.add(meta);
                    }
                    if(Filter.is_zar_file(file.getName())){
                        Emulator.GameInfo meta=Emulator.get.meta_info_from_zar_game(context,file.getUri().toString());
                        if(meta!=null) metas.add(meta);
                    }
                    else if(Filter.is_god_game(file.getName())){
                        Emulator.GameInfo meta=Emulator.get.meta_info_from_god_game(context,file.getUri().toString());
                        if(meta!=null)
                        metas.add(meta);
                    }
                }
            }

            return metas;
        }


        public Emulator.GameInfo getMetaInfo(int pos){
            return metas.get(pos);
        }



        @Override
        public int getCount(){
            return metas.size();
        }

        @Override
        public Object getItem(int p1){
            return null;
        }

        @Override
        public long getItemId(int p1){
            return p1;
        }

        private LayoutInflater getLayoutInflater(){
            return (LayoutInflater)context_.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        /*Bitmap to_gray_bmp(Bitmap src){
            int w=src.getWidth();
            int h=src.getHeight();
            Bitmap bmp=Bitmap.createBitmap(w,h,src.getConfig());
            ColorMatrix cm=new ColorMatrix();
            cm.setSaturation(0);
            Canvas canvas=new Canvas(bmp);
            Paint paint=new Paint();
            paint.setColorFilter(new ColorMatrixColorFilter(cm));
            canvas.drawBitmap(src,0,0,paint);
            return bmp;
        }*/

        @Override
        public View getView(int pos,View curView,ViewGroup p3){

            if(curView==null){
                curView=getLayoutInflater().inflate(R.layout.game_item,null);
            }

            Emulator.GameInfo mi=metas.get(pos);

            ImageView icon=(ImageView)curView.findViewById(R.id.game_icon);
            if(mi.icon==null)
                icon.setImageResource(R.drawable.app_icon);
            else {
                Bitmap icon_bmp= BitmapFactory.decodeByteArray(mi.icon,0,mi.icon.length);
                icon.setImageBitmap(icon_bmp);
            }

            TextView name=(TextView)curView.findViewById(R.id.game_name);
            if(mi.name!=null)
                name.setText(mi.name);
            else{
                DocumentFile file=DocumentFile.fromSingleUri(context_,Uri.parse(mi.uri));
                name.setText(file.getName());
            }

            return curView;
        }
    }//!FileAdapter
}