import { SubHeading } from '../../components';
import styles from './Chef.module.css';

const Chef = () => {
  return (
    <div className="app__bg app__wrapper section__padding">
      <div className="app__wrapper_img app__wrapper_img-reverse">
        <img src="/chef.png" alt="chef" />
      </div>

      <div className="app__wrapper_info">
        <SubHeading title="Chef's Word" />
        <h1 className="headtext__cormorant">What We Believe In</h1>

        <div className={styles.app__chef_content}>
          <div className={styles.app__chef_content_quote}>
            <img src="/quote.png" alt="quote" />
            <p className="p__opensans">
              Colletta (collection) is a compilation of lively dining experiences.
            </p>
          </div>
          <p className="p__opensans">
            Our creative, food and beverage program combines satisfying staples with
            imaginative twists. From boozy brunches and happy hours to family dinners,
            special occasions and everything in between.
          </p>
        </div>

        <div className={styles.app__chef_sign}>
          <p>Kevin Luo</p>
          <p className="p__opensans">Chef & Founder</p>
          <img src="/sign.png" alt="sign" />
        </div>
      </div>
    </div>
  );
};

export default Chef;
