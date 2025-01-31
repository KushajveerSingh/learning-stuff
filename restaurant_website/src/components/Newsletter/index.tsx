import styles from './Newsletter.module.css';
import { SubHeading } from '../';

const Newsletter = () => {
  return (
    <div className={styles.app__newsletter}>
      <div className={styles.app__newsletter_heading}>
        <SubHeading title="Newsletter" />
        <h1 className="headtext__cormorant">Subscribe To Our Newsletter</h1>
        <p className="p__opensans">And never miss latest updates!</p>
      </div>

      <div className={`${styles.app__newsletter_input} flex__center`}>
        <input type="email" placeholder="Enter your email address" />
        <button className="custom__button">Subscribe</button>
      </div>
    </div>
  );
};

export default Newsletter;
