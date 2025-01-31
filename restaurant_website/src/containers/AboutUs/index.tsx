import styles from './AboutUs.module.css';

const AboutUs = () => {
  return (
    <div
      className={`${styles.app__aboutus} app__bg flex__center section__padding`}
      id="about"
    >
      <div className={`${styles.app__aboutus_overlay} flex__center`}>
        <img src="/G.png" alt="g letter" />
      </div>

      <div className={`${styles.app__aboutus_content} flex__center`}>
        <div className={styles.app__aboutus_content_about}>
          <h1 className="headtext__cormorant">About Us</h1>
          <img src="/spoon.png" alt="about_spoon" className="spoon_img" />
          <p className="p__opensans">
            The Original -- The OG -- reinvents the neighborhood eatery, combining
            timeless inspiration from classic Americana with inventive and playful modern
            flair.
          </p>
          <button type="button" className="custom__button">
            Know More
          </button>
        </div>

        <div className={`${styles.app__aboutus_content_knife} flex__center`}>
          <img src="/knife.png" alt="about knife" />
        </div>

        <div className={styles.app__aboutus_content_history}>
          <h1 className="headtext__cormorant">Our History</h1>
          <img src="/spoon.png" alt="about_spoon" className="spoon_img" />
          <p className="p__opensans">
            With influences ranging from the 1950s diner to the elegant railway dining
            cars of days past, you&apos;ll find a nostalgic collection of comforts,
            classics with a modern twist.
          </p>
          <button type="button" className="custom__button">
            Know More
          </button>
        </div>
      </div>
    </div>
  );
};

export default AboutUs;
