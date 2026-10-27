// SPDX-License-Identifier: WTFPL
package aenu.ax360e;

import android.content.Context;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import androidx.documentfile.provider.DocumentFile;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.Map;

public class Emulator extends aenu.emulator.Emulator{
    public static Emulator get=null;
    public static void load_library(){
        if(get!=null)
            throw new RuntimeException("Emulator already loaded");
        get=new Emulator();
        System.loadLibrary("e");
    }

    /*public void key_event(int keycode,boolean pressed){
        throw new RuntimeException("Not implemented");
        final int unused=-1;
        super.key_event(keycode,pressed,unused);
    }*/

    public native void setup_context(Context ctx);
    public native void setup_document_file_tree(DocumentFile tree);
    public native void setup_launch_args(String[] args);
    public native String simple_device_info();
    public native String generate_config_xml(String config_path);
    public static int nc_open_uri_fd(Context ctx,Uri uri) {
        try {
            ParcelFileDescriptor pfd_ = ctx.getContentResolver().openFileDescriptor(uri, "r");
            int game_fd=pfd_.detachFd();
            pfd_.close();
            return game_fd;
        } catch (Exception e) {
            Log.e("ax360e",e.toString());
            return -1;
        }
    }


    public native GameInfo meta_info_from_god_game(Context ctx,String uri) throws RuntimeException;
    public native GameInfo meta_info_from_iso_game(Context ctx,String uri) throws RuntimeException;
    public native GameInfo meta_info_from_zar_game(Context ctx,String uri) throws RuntimeException;
    public native GameInfo meta_info_from_xex_game(Context ctx,String boot_xex_uri) throws RuntimeException;

    /* Xbox 360 用户档案管理 (native 实现, 布局与 xenia ProfileManager 一致) */
    //返回多行 "XUID16\tGamertag"
    public native String xe_profile_list(String content_root);
    public native boolean xe_profile_create(String content_root,String gamertag);
    public native boolean xe_profile_rename(String content_root,String xuid,String gamertag);
    public native boolean xe_profile_delete(String content_root,String xuid);

    //与 ProfileManager::IsGamertagValid 相同的规则
    public static boolean is_valid_gamertag(String gamertag){
        if(gamertag==null) return false;
        return gamertag.matches("^[A-Za-z][A-Za-z0-9]*( [A-Za-z0-9]+)*$")
                &&gamertag.length()>=1&&gamertag.length()<=15;
    }

    public static class GameInfo{

        public String uri;
        public String name;
        public int fd;
        public byte[] icon;
        public String title_id;


        static JSONObject to_json(GameInfo  info) throws JSONException {
            JSONObject json=new JSONObject();

            json.put("uri",info.uri);
            if(info.name!=null)
                json.put("name",info.name);

            if(info.icon!=null)
                json.put("icon", Base64.getEncoder().encodeToString(info.icon));
            if(info.title_id!=null)
                json.put("title_id",info.title_id);
            return json;
        }

        static GameInfo from_json(JSONObject json) throws JSONException {
            GameInfo info=new GameInfo();
            info.uri=json.getString("uri");
            if(json.has("name"))
                info.name=json.getString("name");
            if(json.has("icon"))
                info.icon=Base64.getDecoder().decode(json.getString("icon"));
            if(json.has("title_id"))
                info.title_id=json.getString("title_id");

            return info;
        }
    }

    static class PatchManager {

        public static class PatchInfo {
            public String name;
            public int index;

            public boolean isEnabled;

            public PatchInfo(String name,int index, boolean isEnabled) {
                this.name = name;
                this.index = index;
                this.isEnabled = isEnabled;
            }
        }

        static class PatchContext {
            String file_name=null;
            Config config = null;
            List<PatchManager.PatchInfo> patches=new ArrayList<>();
        }
        static void find_patches_context(String title_id,PatchContext out_patch_packet) throws ConfigFileException, IOException {
            File patches_dir = Application.get_patches_dir();

            //patches
            if(patches_dir.exists()){
                File[] files=patches_dir.listFiles();
                if(files!=null){
                    for(File file:files){
                        String file_name=file.getName();
                        if(file_name.startsWith(title_id)&&file_name.endsWith(".patch.toml")){
                            out_patch_packet.file_name=file_name;
                            out_patch_packet.config=Config.open_config_from_string(Utils.read_file_as_str(file));
                            return;
                        }
                    }
                }
            }

            //assets
            String[] files = Application.ctx.getAssets().list("game-patches");
            if(files!=null){
                for(String file_name:files){
                    if(file_name.startsWith(title_id)&&file_name.endsWith(".patch.toml")){
                        String config_str = new String(Utils.load_assets_file(Application.ctx,"game-patches/"+file_name));
                        out_patch_packet.file_name=file_name;
                        out_patch_packet.config=Config.open_config_from_string(config_str);
                        return;
                    }
                }
            }
        }

        public static PatchContext getPatchContextForGame(String title_id) {

            PatchManager.PatchContext ctx = new PatchManager.PatchContext();
            try {
                find_patches_context(title_id, ctx);
            } catch (Exception e) {
                return null;
            }

            if(ctx.config==null) return null;

            int patchCount = ctx.config.load_config_tab_arr_size("patch");

            for (int i = 0; i < patchCount; i++) {
                String name   = ctx.config.load_config_tab_arr_entry("patch|name", i);
                String enabled = ctx.config.load_config_tab_arr_entry("patch|is_enabled", i);
                ctx.patches.add(new PatchManager.PatchInfo(name, i, Boolean.valueOf( enabled)));
            }

            return ctx;
        }

        public static void updatePatchStatus(PatchManager.PatchContext ctx,PatchManager.PatchInfo patch, boolean isEnabled) {
            patch.isEnabled = isEnabled;
            int patchIndex = patch.index;
            ctx.config.save_config_tab_arr_entry("patch|is_enabled", patchIndex, Boolean.toString(isEnabled));
        }

        public static void savePatchFile(PatchManager.PatchContext ctx) {
            String config_str = ctx.config.close_config();
            if(!Application.get_patches_dir().exists()) Application.get_patches_dir().mkdirs();
            Utils.save_string(new File(Application.get_patches_dir(), ctx.file_name), config_str);
        }
    }
}
