import { useRef, useState } from 'react';
import styles from './Intro.module.css';
import { BsFillPlayFill, BsPauseFill } from 'react-icons/bs';

const Intro = () => {
  const [playVideo, setPlayVideo] = useState(false);
  const videoRef = useRef<HTMLVideoElement>(null);

  const handleVideo = () => {
    setPlayVideo(!playVideo);

    if (videoRef.current !== null) {
      if (playVideo) {
        videoRef.current.pause();
      } else {
        videoRef.current.play();
      }
    }
  };

  return (
    <div className={styles.app__video}>
      <video src="/meal.mp4" loop controls={false} muted ref={videoRef} />

      <div className={`${styles.app__video_overlay} flex__center`}>
        <div
          className={`${styles.app__video_overlay_circle} flex__center`}
          onClick={handleVideo}
        >
          {playVideo ? (
            <BsPauseFill color="#fff" fontSize={30} />
          ) : (
            <BsFillPlayFill color="#fff" fontSize={30} />
          )}
        </div>
      </div>
    </div>
  );
};

export default Intro;
