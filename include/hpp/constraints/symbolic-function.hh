// Copyright (c) 2015, LAAS-CNRS
// Authors: Joseph Mirabel (joseph.mirabel@laas.fr)
//
// This file is part of hpp-constraints.
// hpp-constraints is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-constraints is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-constraints. If not, see <http://www.gnu.org/licenses/>.

#ifndef HPP_CONSTRAINTS_SYMBOLIC_FUNCTION_HH
# define HPP_CONSTRAINTS_SYMBOLIC_FUNCTION_HH

# include <hpp/constraints/fwd.hh>
# include <hpp/constraints/config.hh>
# include <hpp/constraints/symbolic-calculus.hh>
# include <hpp/constraints/differentiable-function.hh>

# include <hpp/pinocchio/device.hh>
# include <hpp/pinocchio/liegroup-element.hh>

namespace hpp {
  namespace constraints {

    /// Wrapper of a CalculusBaseAbstract derived object into a DifferentiableFunction
    /// \note At the moment, it is not possible to write a CalculusBaseAbstract
    ///       derived object that uses the extra config space of the robot.
    template <typename Expression>
    class HPP_CONSTRAINTS_DLLAPI SymbolicFunction : public DifferentiableFunction
    {
      public:
        typedef shared_ptr<SymbolicFunction> Ptr_t;
        typedef weak_ptr<SymbolicFunction> WkPtr_t;

        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        /// Return a shared pointer to a new instance
        static Ptr_t create (
            const std::string& name, const DevicePtr_t& robot,
            const typename Traits<Expression>::Ptr_t expr)
        {
          std::vector <bool> mask (expr->value ().size (), true);
          return create (name, robot, expr, mask);
        }

        /// Return a shared pointer to a new instance
        static Ptr_t create (
            const std::string& name, const DevicePtr_t& robot,
            const typename Traits<Expression>::Ptr_t expr,
            std::vector <bool> mask)
        {
          assert (mask.size() == (std::size_t) expr->value().size());
          Ptr_t ptr (new SymbolicFunction (name,robot,expr,mask));
          ptr->init (ptr);
          return ptr;
        }

        /// Return a shared pointer to a new instance
        static Ptr_t create (const std::string& name, size_type sizeInput,
                          size_type sizeInputDerivative,
                          size_type sizeOutput,
                          const typename Traits<Expression>::Ptr_t expr)
        {
          std::vector<bool> mask (sizeOutput, true);
          Ptr_t ptr (new SymbolicFunction (name,sizeInput,sizeInputDerivative,sizeOutput,expr,mask));
          ptr->init (ptr);
          return ptr;
        }

        virtual ~SymbolicFunction () {}

        SymbolicFunction (const std::string& name, const DevicePtr_t& robot,
                          const typename Traits<Expression>::Ptr_t expr,
                          std::vector <bool> mask) :
          DifferentiableFunction (robot->configSize(), robot->numberDof(),
                                  LiegroupSpace::Rn (expr->value().size()),
                                  name),
          robot_ (robot), expr_ (expr), mask_ (mask)
        {
          size_type d = robot_->extraConfigSpace().dimension();
          activeParameters_          .tail(d).setConstant(false);
          activeDerivativeParameters_.tail(d).setConstant(false);
        }

        SymbolicFunction (const std::string& name, size_type sizeInput,
                          size_type sizeInputDerivative,
                          size_type sizeOutput,
                          const typename Traits<Expression>::Ptr_t expr,
                          std::vector <bool> mask) :
          DifferentiableFunction (sizeInput, sizeInputDerivative,
                                  LiegroupSpace::Rn (sizeOutput), name),
          robot_ (), expr_ (expr), mask_ (mask) {}

      protected:
        /// Compute value of error
        ///
        /// \param argument configuration of the robot,
        /// \retval result error vector
        virtual void impl_compute (LiegroupElementRef result,
                                   ConfigurationIn_t argument) const
        {
          if (robot_) {
            robot_->currentConfiguration (argument);
            robot_->computeForwardKinematics ();
          }
          expr_->invalidate ();
          expr_->computeValue (argument);
          size_t index = 0;
          for (std::size_t i = 0; i < mask_.size (); i++) {
            if (mask_[i])
              result.vector () [index++] = expr_->value () [i];
          }
        }

        virtual void impl_jacobian (matrixOut_t jacobian,
            ConfigurationIn_t arg) const
        {
          if (robot_) {
            robot_->currentConfiguration (arg);
            robot_->computeForwardKinematics ();
          }
          expr_->invalidate ();
          expr_->computeJacobian (arg);
          size_t index = 0;

          const typename Expression::JacobianType_t& Je (expr_->jacobian());
          size_type nv;
          if (robot_) {
            size_type d = robot_->extraConfigSpace().dimension();
            nv = Je.cols();
            assert(robot_->numberDof() == Je.cols() + d);
            jacobian.rightCols(d).setZero();
          } else {
            nv = jacobian.cols();
          }

          for (std::size_t i = 0; i < mask_.size (); i++) {
            if (mask_[i])
              jacobian.row(index++).head(nv) = Je.row (i);
          }
        }

        void init (const Ptr_t& self) {
          wkPtr_ = self;
        }
      private:
        WkPtr_t wkPtr_;
        DevicePtr_t robot_;
        typename Traits<Expression>::Ptr_t expr_;
        std::vector <bool> mask_;
    }; // class ComBetweenFeet
  } // namespace constraints
} // namespace hpp
#endif // HPP_CONSTRAINTS_SYMBOLIC_FUNCTION_HH
