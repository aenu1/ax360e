package aenu.ax360e;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.InputDevice;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.documentfile.provider.DocumentFile;

import java.io.File;

// Created by aenu on 2025/7/29.
// SPDX-License-Identifier: WTFPL
public class EmulatorActivity extends Activity implements SurfaceHolder.Callback, View.OnGenericMotionListener {

    static final int DELAY_ON_CREATE=0xaeae0001;
    public static final String EXTRA_GAME_URI="game_uri";
    public static final String EXTRA_CONFIG_PATH="config_path";
    static SurfaceView sf=null;
    // Soft keyboard support for the ImGui keyboard input dialog.
    static EmulatorActivity g_activity=null;
    EditText soft_input_view=null;
    InputMethodManager imm=null;

    public static void show_soft_input(final String initial){
        final EmulatorActivity act=g_activity;
        if(act==null||act.soft_input_view==null) return;
        act.runOnUiThread(new Runnable(){
            @Override
            public void run(){
                act.soft_input_view.setText(initial==null?"":initial);
                act.soft_input_view.setSelection(act.soft_input_view.getText().length());
                act.soft_input_view.requestFocus();
                if(act.imm!=null)
                    act.imm.showSoftInput(act.soft_input_view,InputMethodManager.SHOW_IMPLICIT);
            }
        });
    }

    public static void hide_soft_input(){
        final EmulatorActivity act=g_activity;
        if(act==null||act.soft_input_view==null) return;
        act.runOnUiThread(new Runnable(){
            @Override
            public void run(){
                if(act.imm!=null)
                    act.imm.hideSoftInputFromWindow(act.soft_input_view.getWindowToken(),0);
                act.soft_input_view.clearFocus();
                // Give gamepad key events back to the emulator surface.
                if(sf!=null) sf.requestFocus();
            }
        });
    }

    void setup_soft_input(){
        g_activity=this;
        imm=(InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);
        soft_input_view=new EditText(this);
        // Invisible 1x1 widget used only to host the IME connection.
        soft_input_view.setLayoutParams(new FrameLayout.LayoutParams(1,1));
        soft_input_view.setAlpha(0f);
        soft_input_view.setBackgroundColor(0);
        soft_input_view.setInputType(InputType.TYPE_CLASS_TEXT);
        soft_input_view.setImeOptions(EditorInfo.IME_ACTION_DONE);
        soft_input_view.addTextChangedListener(new TextWatcher(){
            @Override public void beforeTextChanged(CharSequence s,int a,int b,int c){}
            @Override public void onTextChanged(CharSequence s,int a,int b,int c){}
            @Override public void afterTextChanged(Editable s){
                // Live-sync the typed text into the ImGui dialog buffer.
                Emulator.get.ime_input(s.toString(),false);
            }
        });
        soft_input_view.setOnEditorActionListener(new TextView.OnEditorActionListener(){
            @Override
            public boolean onEditorAction(TextView v,int actionId,KeyEvent event){
                if(actionId==EditorInfo.IME_ACTION_DONE||actionId==EditorInfo.IME_NULL){
                    Emulator.get.ime_input(v.getText().toString(),true);
                    hide_soft_input();
                    return true;
                }
                return false;
            }
        });
        // After the IME has been dismissed (e.g. via back), hand mapped
        // gamepad keys back to the emulator instead of eating them here.
        soft_input_view.setOnKeyListener(new View.OnKeyListener(){
            @Override
            public boolean onKey(View v,int keyCode,KeyEvent event){
                if(imm!=null&&imm.isAcceptingText()) return false;
                int gameKey=keysMap.get(keyCode,KEY_NO_MAPPED);
                if(gameKey==KEY_NO_MAPPED) return false;
                if(event.getAction()==KeyEvent.ACTION_DOWN){
                    if(event.getRepeatCount()==0)
                        Emulator.get.key_event(gameKey,true,VirtualControl.KEY_VALUE_UNUSED);
                    if(sf!=null) sf.requestFocus();
                    return true;
                }else if(event.getAction()==KeyEvent.ACTION_UP){
                    Emulator.get.key_event(gameKey,false,VirtualControl.KEY_VALUE_UNUSED);
                    if(sf!=null) sf.requestFocus();
                    return true;
                }
                return false;
            }
        });
        ViewGroup root=(ViewGroup)sf.getParent();
        root.addView(soft_input_view);
    }
    private SparseIntArray keysMap = new SparseIntArray();
    private Vibrator vibrator=null;
    private VibrationEffect vibrationEffect=null;
    boolean started=false;
    Dialog delay_dialog=null;
    final Handler delay_on_create=new Handler(new Handler.Callback(){
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
    void on_create(){
        String uri=getIntent().getStringExtra(EXTRA_GAME_URI);
        aenu.emulator.Emulator.Path path=aenu.emulator.Emulator.Path.from(uri,-1);
        String config_path=getIntent().getStringExtra(EXTRA_CONFIG_PATH);
        if(config_path==null) config_path=Application.get_global_config_file().getAbsolutePath();

        Emulator.get.setup_context(this);
        Emulator.get.setup_document_file_tree(DocumentFile.fromTreeUri(this,MainActivity.load_pref_game_dir( this)));
        Emulator.get.setup_game_path(path);
        Emulator.get.setup_launch_args(new String[]{
                "--storage_root="+Application.get_xe_storage_root().getAbsolutePath(),
                "--config="+config_path,
                "--log_file="+Application.get_app_data_dir().getAbsolutePath()+"/xe.log",
                /*"--storage_root=/storage/emulated/0/Download/ax360e",
                "--log_file=/storage/emulated/0/Download/ax360e/xe.log",*/
        });
        setContentView(R.layout.activity_emulator);
        sf = (SurfaceView) findViewById(R.id.surface_view);
        sf.getHolder().addCallback(EmulatorActivity.this);

        sf.setFocusable(true);
        sf.setFocusableInTouchMode(true);
        sf.requestFocus();
        sf.setOnGenericMotionListener(this);

        setup_soft_input();

        load_key_map_and_vibrator();
    }
    void vibrator(){
        if(vibrator!=null) {
            vibrator.vibrate(vibrationEffect);
        }
    }

    void load_key_map_and_vibrator() {
        final SharedPreferences sPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        keysMap.clear();
        for (int i = 0; i < KeyMapConfig.KEY_NAMEIDS.length; i++) {
            String keyName = Integer.toString(KeyMapConfig.KEY_NAMEIDS[i]);
            int keyCode = sPrefs.getInt(keyName, KeyMapConfig.DEFAULT_KEYMAPPERS[i]);
            keysMap.put(keyCode, KeyMapConfig.KEY_VALUES[i]);
        }
        if(sPrefs.getBoolean("enable_vibrator",false)){
            vibrator = (Vibrator) getSystemService(VIBRATOR_SERVICE);
            vibrationEffect = VibrationEffect.createOneShot(25, VibrationEffect.DEFAULT_AMPLITUDE);
        }
    }
    @Override
    protected void onCreate(android.os.Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if(!Application.should_delay_load()){
            on_create();
            return;
        }

        delay_dialog=ProgressTask.create_progress_dialog( this,getString(R.string.loading));
        delay_dialog.show();
        new Thread() {
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
        return;
    }

    @Override
    public void onBackPressed()
    {

        if(delay_dialog!=null||quitting)
            return;

        // A managed dialog is up (possibly still mid-attach, before its
        // window could consume this BACK itself): close it. The dismiss
        // bookkeeping resumes the game.
        if(menu_dialog!=null){ close_menu(); return; }

        show_pause_menu();
    }

    // ============ pause ownership (XenDroid multi-owner pattern) ============
    // The game runs only while NO owner wants it paused. Owners:
    //   owner_menu      - in-game menu dialog up (dialog shown = paused)
    //   owner_background- activity stopped (onStop/onStart)
    //   owner_surface   - the render surface is gone; separate from
    //                     owner_background because onStart can run BEFORE
    //                     surfaceCreated recreates the surface - resuming in
    //                     that gap wakes the pipeline into a dead swapchain
    //   owner_focus     - window focus lost to ANY other window (dialogs,
    //                     system prompts), debounced 250ms
    // Pause/Resume are the XenDroid SOFT pause: park the audio/GPU workers
    // only (fast, synchronous, boot-safe). Repeated calls are no-ops.
    volatile boolean owner_menu=false;
    volatile boolean owner_background=false;
    volatile boolean owner_surface=false;
    volatile boolean owner_focus=false;
    // finish() was picked - stay paused through teardown, never auto-resume.
    volatile boolean quitting=false;

    final android.os.Handler pause_handler=new android.os.Handler();
    final Runnable focus_pause_run=new Runnable(){
        @Override
        public void run(){
            if(started&&!quitting){
                owner_focus=true;
                sync_pause();
            }
        }
    };

    static final String TAG_PAUSE="ax360e-pause";
    static final int FOCUS_PAUSE_DEBOUNCE_MS=250;

    void sync_pause(){
        if(!started) return;
        final boolean want_pause=owner_menu||owner_background||owner_surface
                ||owner_focus||quitting;
        android.util.Log.d(TAG_PAUSE,"owners menu="+owner_menu+" bg="+owner_background
                +" surface="+owner_surface+" focus="+owner_focus
                +" quitting="+quitting+" -> "
                +(want_pause?"PAUSE":"RESUME"));
        if(want_pause)
            Emulator.get.pause();
        else
            Emulator.get.resume();
    }

    // The pause menu dialog (继续游戏 / 退出).
    AlertDialog menu_dialog=null;

    // Dialog.dismiss() on a dialog that was never attached (BACK raced the
    // show()) does NOT fire the dismiss listener, so the bookkeeping runs
    // manually in that case; menu_dismissed() is idempotent for both paths.
    void close_menu(){
        if(menu_dialog==null) return;
        if(menu_dialog.isShowing()) menu_dialog.dismiss();
        else menu_dismissed();
    }

    void menu_dismissed(){
        if(menu_dialog==null) return;
        menu_dialog=null;
        owner_menu=false;
        sync_pause();
    }

    // BACK opens the in-game menu: pause immediately (no focus-debounce
    // lag); dismissing it resumes the game.
    void show_pause_menu(){
        if(quitting) return;
        if(!started) return;   // still booting - BACK is ignored (XenDroid)
        if(menu_dialog!=null) return;   // reentry guard
        owner_menu=true;
        sync_pause();

        AlertDialog.Builder ab=new AlertDialog.Builder(this);
        ab.setItems(new CharSequence[]{"继续游戏",getString(R.string.quit)}, new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface p1, int which)
            {
                if(which==1){
                    quitting=true;
                    finish();
                }
                else{
                    close_menu();
                }
            }
        });
        AlertDialog d=ab.create();
        d.setOnDismissListener(new DialogInterface.OnDismissListener(){
            @Override
            public void onDismiss(DialogInterface p1)
            {
                menu_dismissed();
            }
        });
        menu_dialog=d;
        d.show();
    }

    @Override
    protected void onStart()
    {
        super.onStart();
        android.util.Log.d(TAG_PAUSE,"activity start");
        // Mirror of onStop: the lifecycle owner clears here, but the SURFACE
        // owner keeps the pause until surfaceCreated - never resume into a
        // swapchain that does not exist yet, and never run the game behind
        // an open menu.
        owner_background=false;
        sync_pause();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
        android.util.Log.d(TAG_PAUSE,"activity stop");
        // Park the audio/GPU workers when the activity leaves the screen
        // (home / app switch / screen off) - audio must not keep playing in
        // the background.
        owner_background=true;
        sync_pause();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        if(hasFocus){
            android.util.Log.d(TAG_PAUSE,"focus gained");
            pause_handler.removeCallbacks(focus_pause_run);
            if(owner_focus){
                owner_focus=false;
                sync_pause();
            }
        }else if(started){
            android.util.Log.d(TAG_PAUSE,"focus lost");
            // Any window on top of ours takes the focus away. Debounce
            // transient losses, then pause: this is what makes
            // "a dialog is shown -> game paused, dialog hidden -> resumed"
            // hold for EVERY window, not just the managed ones.
            pause_handler.removeCallbacks(focus_pause_run);
            pause_handler.postDelayed(focus_pause_run,FOCUS_PAUSE_DEBOUNCE_MS);
        }
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        if(g_activity==this) g_activity=null;
        System.exit(0);
    }

    final int KEY_NO_MAPPED = -1;

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        int gameKey = keysMap.get(keyCode, KEY_NO_MAPPED);
        if (gameKey == KEY_NO_MAPPED) return super.onKeyDown(keyCode, event);
        if (event.getRepeatCount() == 0){
            vibrator();
            Emulator.get.key_event(gameKey, true,VirtualControl.KEY_VALUE_UNUSED);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        int gameKey = keysMap.get(keyCode, KEY_NO_MAPPED);
        if (gameKey != KEY_NO_MAPPED) {
            Emulator.get.key_event(gameKey, false,VirtualControl.KEY_VALUE_UNUSED);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }
    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {

        if(!started){
            started=true;

            Emulator.get.setup_surface(holder.getSurface());
            try {
                Emulator.get.boot();
            } catch (aenu.emulator.Emulator.BootException e) {
                throw new RuntimeException(e);
            }
        }
        else{
            Emulator.get.setup_surface(holder.getSurface());
            Emulator.get.surface_changed();
            // Surface is back (return from background / surface recreate):
            // resume only if no dialog owns the pause - never run the game
            // behind an open menu. Earliest SAFE resume point: the swapchain
            // target exists again.
            owner_surface=false;
            sync_pause();
        }

    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        if(!started) return;
        if(width==0||height==0) return;
        Emulator.get.change_surface(width,height);
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        if(!started) return;
        android.util.Log.d(TAG_PAUSE,"surface destroyed");
        // Park the pipeline BEFORE the surface teardown so nothing presents
        // into a released window. owner_surface (not owner_background): it
        // must survive until surfaceCreated - onStart alone must not resume
        // into a dead swapchain.
        owner_surface=true;
        sync_pause();
        Emulator.get.setup_surface(null);
    }


    boolean handle_dpad(InputEvent event) {

        boolean pressed=false;
        if (event instanceof MotionEvent) {

            // Use the hat axis value to find the D-pad direction
            MotionEvent motionEvent = (MotionEvent) event;
            float xaxis = motionEvent.getAxisValue(MotionEvent.AXIS_HAT_X);
            float yaxis = motionEvent.getAxisValue(MotionEvent.AXIS_HAT_Y);

            // Check if the AXIS_HAT_X value is -1 or 1, and set the D-pad
            // LEFT and RIGHT direction accordingly.
            if (Float.compare(xaxis, -1.0f) == 0) {
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_LEFT, true,VirtualControl.KEY_VALUE_UNUSED);
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_RIGHT, false,VirtualControl.KEY_VALUE_UNUSED);
                vibrator();
                pressed=true;
            } else if (Float.compare(xaxis, 1.0f) == 0) {
                Emulator.get.key_event( VirtualControl.KEY_CODE_DPAD_RIGHT, true,VirtualControl.KEY_VALUE_UNUSED);
                Emulator.get.key_event( VirtualControl.KEY_CODE_DPAD_LEFT, false,VirtualControl.KEY_VALUE_UNUSED);

                vibrator();
                pressed=true;
            }
            // Check if the AXIS_HAT_Y value is -1 or 1, and set the D-pad
            // UP and DOWN direction accordingly.
            if (Float.compare(yaxis, -1.0f) == 0) {
                Emulator.get.key_event(  VirtualControl.KEY_CODE_DPAD_UP, true,VirtualControl.KEY_VALUE_UNUSED);
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_DOWN, false,VirtualControl.KEY_VALUE_UNUSED);

                vibrator();
                pressed=true;
            } else if (Float.compare(yaxis, 1.0f) == 0) {
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_DOWN, true,VirtualControl.KEY_VALUE_UNUSED);
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_UP, false,VirtualControl.KEY_VALUE_UNUSED);

                vibrator();
                pressed=true;
            }
        }
        else if (event instanceof KeyEvent) {

            // Use the key code to find the D-pad direction.
            KeyEvent keyEvent = (KeyEvent) event;
            if (keyEvent.getKeyCode() == KeyEvent.KEYCODE_DPAD_LEFT) {
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_LEFT, true,VirtualControl.KEY_VALUE_UNUSED);
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_RIGHT, false,VirtualControl.KEY_VALUE_UNUSED);

                vibrator();
                pressed=true;

            } else if (keyEvent.getKeyCode() == KeyEvent.KEYCODE_DPAD_RIGHT) {
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_RIGHT, true,VirtualControl.KEY_VALUE_UNUSED);
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_LEFT, false,VirtualControl.KEY_VALUE_UNUSED);

                vibrator();
                pressed=true;

            } else if (keyEvent.getKeyCode() == KeyEvent.KEYCODE_DPAD_UP) {
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_UP, true,VirtualControl.KEY_VALUE_UNUSED);
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_DOWN, false,VirtualControl.KEY_VALUE_UNUSED);

                vibrator();
                pressed=true;

            } else if (keyEvent.getKeyCode() == KeyEvent.KEYCODE_DPAD_DOWN) {
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_DOWN, true,VirtualControl.KEY_VALUE_UNUSED);
                Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_UP, false,VirtualControl.KEY_VALUE_UNUSED);

                vibrator();
                pressed=true;

            }
        }

        if(pressed) return true;
        Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_LEFT, false,VirtualControl.KEY_VALUE_UNUSED);
        Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_UP, false,VirtualControl.KEY_VALUE_UNUSED);
        Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_RIGHT, false,VirtualControl.KEY_VALUE_UNUSED);
        Emulator.get.key_event(VirtualControl.KEY_CODE_DPAD_DOWN, false,VirtualControl.KEY_VALUE_UNUSED);
        return false;
    }


    private static boolean isDpadDevice(MotionEvent event) {
        // Check that input comes from a device with directional pads.
        if ((event.getSource() & InputDevice.SOURCE_DPAD)
                != InputDevice.SOURCE_DPAD) {
            return true;
        } else {
            return false;
        }
    }

    @Override
    public boolean onGenericMotion(View v, MotionEvent event) {

        if(isDpadDevice(event)&& handle_dpad(event)) return true;

        if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK/*&&
			event.getAction() == MotionEvent.ACTION_MOVE*/) {
            float laxisX = event.getAxisValue(MotionEvent.AXIS_X);
            float laxisY = event.getAxisValue(MotionEvent.AXIS_Y);
            float raxisX = event.getAxisValue(MotionEvent.AXIS_Z);
            float raxisY = event.getAxisValue(MotionEvent.AXIS_RZ);

            final short _0=0;

            //左摇杆
            {
                if(laxisX!=0){
                    if(laxisX<0){
                        Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_RIGHT,false,_0);
                        Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_LEFT,true,(short) (laxisX*32768.0f));
                    }
                    else{
                        Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_LEFT,false,_0);
                        Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_RIGHT,true,(short)(Math.abs(laxisX)*32767.0f));
                    }
                }
                else{
                    Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_RIGHT,false,_0);
                    Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_LEFT,false,_0);
                }

                //Joystick 左上角为-1.0,-1.0
                //X360左下角为 -32768,-32768
                if(laxisY!=0){
                    if(laxisY<0){
                        Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_DOWN,false,_0);
                        Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_UP,true,(short)(-laxisY*32767.0f));
                    }else{
                        Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_UP,false,_0);
                        Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_DOWN,true,(short)(-laxisY*32768.0f));
                    }
                }
                else{
                    Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_DOWN,false,_0);
                    Emulator.get.key_event(VirtualControl.KEY_CODE_LTHUMB_UP,false,_0);
                }
            }
            //右摇杆
            {
                if(raxisX!=0){
                    if(raxisX<0){
                        Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_RIGHT,false,_0);
                        Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_LEFT,true,(short)(raxisX*32768.f));
                    }else{
                        Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_LEFT,false,_0);
                        Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_RIGHT,true,(short)(raxisX*32767.0f));
                    }
                }
                else{
                    Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_RIGHT,false,_0);
                    Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_LEFT,false,_0);
                }

                //Joystick 左上角为-1.0,-1.0
                //X360左下角为 -32768,-32768
                if(raxisY!=0){
                    if(raxisY<0){
                        Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_DOWN,false,_0);
                        Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_UP,true,(short)(-raxisY*32767.0f));
                    }else{
                        Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_UP,false,_0);
                        Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_DOWN,true,(short)(-raxisY*32768.0f));
                    }
                }
                else{
                    Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_DOWN,false,_0);
                    Emulator.get.key_event(VirtualControl.KEY_CODE_RTHUMB_UP,false,_0);
                }
            }
            return true;
        }

        return super.onGenericMotionEvent(event);
    }

}
